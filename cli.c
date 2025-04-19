/**
 * @file cli.c
 * @brief Implementation of Command Line Interface (CLI) using FreeRTOS and UART.
 *
 * @details
 * This file contains the implementation of the Command Line Interface (CLI) system.
 * The CLI is built using FreeRTOS tasks and queues to handle user commands efficiently.
 * It receives input via UART using an interrupt-driven approach and processes commands
 * registered through FreeRTOS+CLI.
 *
 * The system supports asynchronous command execution, UART-based input handling,
 * and structured output responses. It ensures smooth CLI operation by managing
 * buffers, task synchronization, and error handling.
 *
 * @date Created on 24.03.2025
 * @author Yauheni Bialkou
 */

//======================================================================[ INCLUDE ]======================================================================================================== //

#include "cli.h"
#include "cli_cmd.h"
#include <stdio.h>
#include <string.h>

//======================================================================[ INTERNAL MACRO DEFENITIONS ]===================================================================================== //

//======================================================================[ INTERNAL DATA TYPES DEFINITIONS ]================================================================================ //

//======================================================================[ INTERNAL FUNCTIONS AND OBJECTS DECLARATION ]===================================================================== //

static Cli_s cliInstance = {0}; // Instance of CLI structure to store system state

/**
 * @brief CLI task that processes incoming commands.
 *
 * \param[in]  argument - Unused task parameter;
 * \param[out] none.
 */
static void cliTask(void *argument);

/**
 * @brief Configures UART to receive or transmit mode.
 *
 * \param[in]  isTransmit - If true, sets UART to transmit mode; otherwise, receive mode;
 * \param[out] none;
 * \return     none.
 */
static void cliSetUartDirectionMode(Cli_UartMode_e UartMode);

/**
 * @brief UART RX callback function for handling received characters.
 *
 * \param[in]  uart - Pointer to the USART descriptor;
 * \param[out] none;
 * \return     none.
 */
static void cliRxReceivedCb(const struct usart_async_descriptor *const uart);

/**
 * @brief UART TX callback function.
 *
 * \param[in]  uart - Pointer to the USART descriptor;
 * \param[out] none;
 * \return     none.
 */
static void cliTxCompletedCb(const struct usart_async_descriptor *const uart);

/**
 * @brief UART Error callback function.
 *
 * \param[in]  uart - Pointer to the USART descriptor;
 * \param[out] none;
 * \return     none.
 */
static void cliRxTxErr(const struct usart_async_descriptor *const uart);

/**
 * @brief Handles CLI authentication state machine.
 *
 * \param[in]  none;
 * \param[out] none;
 * \return     none.
 */
static void cliAuthenticate(void);

/**
 * @brief Sends a message over UART and waits for completion.
 *
 * \param[in]  message - Pointer to the string to be sent;
 * \param[out] none;
 * \return     none.
 */
static void cliSendMessage(const char *message);

//=======================================================================[PUBLIC INTERFACE FUNCTIONS]===================================================================================== //

/**
 * @brief Initializes the Command Line Interface (CLI).
 *
 * \param[in]  none;
 * \param[out] none;
 * \return int16_t - Returns 0 on successful initialization, or a negative error code on failure.
 */
int16_t CliStartup(void)
{
    int16_t status           = 0;           // A variable for storing the execution status
    int32_t ioResult         = 0;           // A variable for storing the result
    int32_t rxCbStatus       = ERR_NONE;    // A variable for storing the RX callback function
    int32_t txCbStatus       = ERR_NONE;    // A variable for storing the TX callback function
    int32_t errCbStatus      = ERR_NONE;    // A variable for storing the Error callback function
    int32_t uartEnableStatus = ERR_NONE;    // A variable for storing the UART enable status

    do
    {
        /* Reset UART pins to RX mode before thread creation */
        cliSetUartDirectionMode(UART_RX_MODE);

        /* Assign the UART instance to the CLI structure */
        cliInstance.uart = &SERVICE_UART;

        /* Get the I/O descriptor and store it */
        ioResult = usart_async_get_io_descriptor(cliInstance.uart, &cliInstance.io);
        if ((ioResult != ERR_NONE) ||
            (cliInstance.io == NULL))
        {
            break;
        }

        /* Assign the index for tracking position in the receive buffer */
        cliInstance.rxIndex = 0;

        /* Clear RX and TX buffers */
        memset(cliInstance.rxBuffer, 0, CLI_RX_BUFFER_SIZE);
        memset(cliInstance.txBuffer, 0, CLI_TX_BUFFER_SIZE);

        /* Create queues for RX and TX communication */
        cliInstance.rxQueue = xQueueCreate(CLI_QUEUE_LENGTH, sizeof(char));
        cliInstance.txQueue = xQueueCreate(CLI_QUEUE_LENGTH, sizeof(char));

        /* Check if queue creation was successful */
        if ((!cliInstance.rxQueue) ||
            (!cliInstance.txQueue))
        {
            status = -1;
            break;
        }

        /* Initialize CLI commands by registering them with FreeRTOS CLI */
        CliCmdInit();

        /* Register the UART RX, TX, Err callback functions */
        rxCbStatus = usart_async_register_callback(cliInstance.uart, USART_ASYNC_RXC_CB, cliRxReceivedCb);
        txCbStatus = usart_async_register_callback(cliInstance.uart, USART_ASYNC_TXC_CB, cliTxCompletedCb);
        errCbStatus = usart_async_register_callback(cliInstance.uart, USART_ASYNC_ERROR_CB, cliRxTxErr);

        /* Check the success of registration of all callbacks */
        if ((rxCbStatus != ERR_NONE) ||
            (txCbStatus != ERR_NONE) ||
            (errCbStatus != ERR_NONE))
        {
            status = -2;
            break;
        }

        /* Enable UART communication */
        uartEnableStatus = usart_async_enable(cliInstance.uart);
        if (uartEnableStatus != ERR_NONE)
        {
            status = -3;
            break;
        }

        /* Set UART to receive mode (RX) */
        cliSetUartDirectionMode(UART_RX_MODE);

        /* Create the CLI processing task */
        BaseType_t taskStatus = xTaskCreate(cliTask,
                                            "CLI_Task",
                                            512,
                                            NULL,
                                            3,
                                            &cliInstance.taskHandle);

        /* Check taskStatus */
        if (taskStatus != pdPASS)
        {
            status = -4;
            break;
        }

    } while (0);

    return status;
}

//=====================================================================[ PRIVATE FUNCTIONS ]============================================================================================== //

/**
 * @brief CLI task that processes incoming commands.
 *
 * This task continuously reads characters from the RX queue, buffers them,
 * and processes completed commands. Processed output is sent to the TX queue.
 *
 * \param[in]  argument - Unused task parameter;
 * \param[out] none;
 * \return none.
 */
static void cliTask(void *argument)
{
    BaseType_t returnStatus = pdFALSE;

    /* Setting the initial authentication state */
    cliInstance.authState = FSM_LOG_IN;

    /* Infinite loop for CLI processing */
    while (1)
    {

        cliAuthenticate();

        /* Wait for a character from the RX queue (blocks until data is received) */
        if (xQueueReceive(cliInstance.rxQueue, &cliInstance.rxChar, portMAX_DELAY) == pdPASS)
        {

            switch (cliInstance.rxChar)
            {
            case CLI_END_CHAR:
                cliInstance.rxBuffer[cliInstance.rxIndex] = CLI_NULL_CHAR;
                do
                {
                    /* Process the command using FreeRTOS + CLI */
                    returnStatus = FreeRTOS_CLIProcessCommand(cliInstance.rxBuffer,
                                                              cliInstance.txBuffer,
                                                              CLI_TX_BUFFER_SIZE);

                    /* Set UART to transmit mode (TX) */
                    cliSetUartDirectionMode(UART_TX_MODE);

                    /* Send next symbol */
                    int32_t writeStatus = io_write(cliInstance.io,
                                                   (uint8_t *)&cliInstance.txBuffer,
                                                   strlen(cliInstance.txBuffer));

                    char queueBuff = 0;
                    xQueueReceive(cliInstance.txQueue, &queueBuff, 1000);

                    if ((returnStatus == pdFALSE) ||
                        (queueBuff == 2))
                    {
                        break;
                    }
                } while (1);

                /* Set UART to receive mode (RX). */
                cliSetUartDirectionMode(UART_RX_MODE);

                cliInstance.rxIndex = 0; // Reset index for the next command
                break;

            case CLI_BS_CHAR:
                if (cliInstance.rxIndex > 0)
                {
                    cliInstance.rxIndex--;
                    cliInstance.rxBuffer[cliInstance.rxIndex] = CLI_NULL_CHAR;
                }
                break;

            default:
                if (cliInstance.rxIndex < CLI_RX_BUFFER_SIZE - 1)
                {
                    cliInstance.rxBuffer[cliInstance.rxIndex++] = cliInstance.rxChar;
                }
                break;
            }
        }
    }
}

/**
 * @brief Configures UART to receive or transmit mode.
 *
 * This function sets the appropriate GPIO levels to switch UART
 * between reception and transmission modes.
 *
 * \param[in]  UartMode - If UART_RX_MODE, sets UART to receive mode; otherwise, transmit mode;
 * \param[out] none;
 * \return     none.
 */
static void cliSetUartDirectionMode(Cli_UartMode_e UartMode)
{
    switch (UartMode)
    {
    case UART_RX_MODE:
        gpio_set_pin_level(SERVICE_UART_RX_EN, false); // Enable RX
        gpio_set_pin_level(SERVICE_UART_TX_EN, false); // Disable TX
        break;

    case UART_TX_MODE:
        gpio_set_pin_level(SERVICE_UART_RX_EN, true); // Disable RX
        gpio_set_pin_level(SERVICE_UART_TX_EN, true); // Enable TX
        break;

    default:
        ASSERT(0);
        break;
    }
}

/**
 * @brief UART RX callback function.
 *
 * This function is called when a character is received via UART.
 * The received character is placed into the RX queue for processing.
 *
 * \param[in]  uart - Pointer to the USART descriptor;
 * \param[out] none;
 * \return     none.
 */
static void cliRxReceivedCb(const struct usart_async_descriptor *const uart)
{

    do
    {
        /* Check io before calling io_read() */
        if (cliInstance.io == NULL)
        {
            break;
        }

        /* Read one character from UART into receivedChar */
        int32_t readStatus = io_read(cliInstance.io, (uint8_t *)&cliInstance.rxChar, 1);
        if (readStatus <= 0)
        {
            break;
        }

        /* Flag to indicate if a higher-priority task has been unblocked during the ISR */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        /* Try to send the character to the RX queue */
        BaseType_t queueSendStatus = xQueueSendFromISR(cliInstance.rxQueue,
                                                       &cliInstance.rxChar,
                                                       &xHigherPriorityTaskWoken);

        /* Check queue creation status*/
        if (queueSendStatus != pdPASS)
        {
            /* Handle other errors */
            break;
        }

        /* If a higher priority task was woken, request a context switch */
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    } while (0);
}

/**
 * @brief UART TX callback function.
 *
 * This function is called when the UART transmission is completed.
 * It retrieves the next character from the TX queue and sends it via UART.
 *
 * \param[in]  uart - Pointer to the USART descriptor;
 * \param[out] none;
 * \return     none.
 */
static void cliTxCompletedCb(const struct usart_async_descriptor *const uart)
{
    do
    {
        /* Check that the UART I/O descriptor is available */
        if (cliInstance.io == NULL ||
            (cliInstance.uart != uart))
        {
            break;
        }

        /* Message indicating that transmission was completed successfully */
        CliTxStatus_e msg = CLI_TX_COMPLETE;

        /* Try to send the character to the TX queue */
        BaseType_t queueReceiveStatus = xQueueSendFromISR(cliInstance.txQueue,
                                                          (void *)&msg,
                                                          NULL);

        /* Checking if there are characters in the TX queue */
        if (queueReceiveStatus == pdFALSE)
        {
            ASSERT(0);
        }

    } while (0);
}

/**
 * @brief UART Error callback function.
 *
 * This function is called when a UART transmission or reception error occurs.
 * It logs the error and resets UART if necessary.
 *
 * \param[in]  uart - Pointer to the USART descriptor;
 * \param[out] none;
 * \return     none.
 */
static void cliRxTxErr(const struct usart_async_descriptor *const uart)
{
    do
    {
        /* Check that the UART I/O descriptor is available */
        if ((cliInstance.io == NULL) ||
            (cliInstance.uart != uart))
        {
            break;
        }

        /* Message indicating that an error occurred during transmission */
        CliTxStatus_e msg = CLI_MSG_ERR;

        /* Try to send the character to the TX queue */
        BaseType_t queueReceiveStatus = xQueueSendFromISR(cliInstance.txQueue,
                                                          (void *)&msg,
                                                          NULL);

        /* Checking if there are characters in the TX queue */
        if (queueReceiveStatus == pdFALSE)
        {
            ASSERT(0);
        }

    } while (0);
}

/**
 * @brief Sends a message over UART and waits for completion.
 *
 * This function transmits a given message over UART in TX mode,
 * waits until the transmission is fully completed,
 * and then switches UART back to RX mode.
 *
 * \param[in]  message - Pointer to the string to be sent;
 * \param[out] none;
 * \return     none.
 */
static void cliSendMessage(const char *message)
{
    /* Set UART to transmit mode */
    cliSetUartDirectionMode(UART_TX_MODE);

    /* Send the provided message over UART */
    io_write(cliInstance.io, (uint8_t *)message, strlen(message));

    /* Wait until the transmission is fully completed */
    xQueueReceive(cliInstance.txQueue, &cliInstance.txChar, portMAX_DELAY);

    /* Restore UART to receive mode */
    cliSetUartDirectionMode(UART_RX_MODE);
}

/**
 * @brief Handles CLI authentication state machine.
 *
 * This function manages the authentication process for the CLI using a finite state machine (FSM).
 * It prompts the user for a password and verifies input before granting access.
 * If authentication fails, the user is asked to retry.
 *
 * \param[in]  none;
 * \param[out] none;
 * \return     none.
 */
static void cliAuthenticate(void)
{
    do
    {
        switch (cliInstance.authState)
        {
        case FSM_LOG_IN:
            /* Clear the input buffer */
            memset(cliInstance.rxBuffer, 0, sizeof(cliInstance.rxBuffer));

            cliSendMessage(PROMPT_PASSWORD);

            /* Reset buffer index and update state */
            cliInstance.rxIndex = 0;
            cliInstance.authState = FSM_INPUT;
            break;

        case FSM_INPUT:
            /* Check if the user has entered a complete password (ending with newline) */
            if (strchr(cliInstance.rxBuffer, '\n'))
            {
                cliInstance.authState = FSM_PROCESS;
            }
            break;

        case

            /* Remove newline characters from input */
            cliInstance.rxBuffer[strcspn(cliInstance.rxBuffer, "\r\n")] = 0;

            /* Validate password */
            if (strcmp(cliInstance.rxBuffer, PASSWORD) == 0) {
                /* Authentication successful, grant access */
                memset(cliInstance.rxBuffer, 0, sizeof(cliInstance.rxBuffer));
                cliInstance.authState = FSM_LOG_OUT;

                cliSendMessage(AUTH_SUCCESS);
            } else {
                /* Authentication failed, go to error state */
                cliInstance.authState = FSM_ERR;
            } break;

            case FSM_ERR:
            cliSendMessage(AUTH_FAIL);

            /* Reset input buffer and authentication process */
            memset(cliInstance.rxBuffer, 0, sizeof(cliInstance.rxBuffer));
            cliInstance.rxIndex = 0;
            cliInstance.authState = FSM_LOG_IN;
            break;

        default:
            /* Undefined state, reset authentication */
            cliInstance.authState = FSM_ERR;
            break;
        }
    } while (1);
}
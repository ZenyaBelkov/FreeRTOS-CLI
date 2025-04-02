/**
 * @file cli.h
 * @brief Command Line Interface (CLI) module declaration.
 *
 * @details
 * This file contains the declarations for the Command Line Interface (CLI) module.
 * The CLI is implemented using FreeRTOS tasks, queues, and USART for communication.
 * It provides a structured way to handle user commands via UART.
 *
 * The structures and function prototypes defined here manage UART communication,
 * task execution, and buffer handling for CLI input and output processing.
 *
 * @date Created on 24.03.2025
 * @author Yauheni Bialkou
 */

#ifndef CLI_H
#define CLI_H

//================================================================[INCLUDE]================================================================================================================//

#include "FreeRTOS.h"        // FreeRTOS kernel headers for task management, queues, etc.
#include "task.h"            // FreeRTOS task management
#include "queue.h"           // FreeRTOS queue management for UART RX/TX
#include "semphr.h"          // FreeRTOS semaphore management for synchronization
#include "FreeRTOS_CLI.h"    // FreeRTOS CLI API
#include "hal_usart_async.h" // USART asynchronous communication for UART
#include "driver_init.h"     // Hardware initialization functions (depends on your project setup)
#include "atmel_start.h"     // Atmel Start library for peripheral initialization (depends on your project setup)
#include "cli_cmd.h"

//===========================================================[MACRO DEFINITIONS]===========================================================================================================//

#define CLI_RX_BUFFER_SIZE 256 // The size of the buffer used for receiving data over UART
#define CLI_TX_BUFFER_SIZE 256 // The size of the buffer used for transmitting data over UART
#define CLI_QUEUE_LENGTH 10    // The size of the queue used for holding incoming and outgoing data

#define CLI_END_CHAR 0x0D  // The character for completing the command input (Carriage Return, CR)
#define CLI_BS_CHAR 0x7F   // ASCII Backspace character code (deleting the last entered character)
#define CLI_NULL_CHAR 0x00 // ASCII code of the null Character (Null Character, '\\0')

#define PASSWORD "1234"
#define PROMPT_PASSWORD "Enter password:"
#define AUTH_SUCCESS "Authentication is successfull!\n"
#define AUTH_FAIL "Authentication error. Try again.\n"

//========================================================[DATA TYPES DEFINITIONS]=========================================================================================================//

/*
 * @brief Enumeration for CLI operation statuses.
 *
 * This enumeration defines the possible status codes returned
 * by CLI functions. These statuses help indicate success or
 * failure conditions in CLI operations.
 */
typedef enum
{
    CLI_STATUS_OK = 0,            // Operation successful
    CLI_STATUS_QUEUE_FULL,        // RX queue is full
    CLI_STATUS_INVALID_PARAMETER, // Invalid parameter
    CLI_STATUS_UART_READ_FAIL,    // UART read operation failed
    CLI_STATUS_UNKNOWN_ERROR      // An unknown error occurred

} Cli_Status_e;

/**
 * @brief Enumeration for UART operating modes.
 *
 * This enumeration defines the modes for configuring UART to either
 * receive or transmit data. The appropriate mode should be set before
 * performing UART operations to ensure correct functionality.
 */
typedef enum
{
    UART_RX_MODE = 0, // Sets UART to receive mode
    UART_TX_MODE      // Sets UART to transmit mode
} Cli_UartMode_e;

/**
 * @brief Enumeration for UART transmission status.
 *
 * This enumeration defines the status codes used to indicate the
 * completion or error state of a UART transmission. It is used
 * within the CLI system to manage UART communication events.
 */
typedef enum
{
    CLI_TX_COMPLETE = 1, // UART transmission was completed successfully
    CLI_MSG_ERR = 2      // UART transmission error occurred
} CliTxStatus_e;

/**
 * @brief Enumeration for authentication FSM states.
 *
 * This enumeration defines the states of the Finite State Machine (FSM)
 * used to manage user authentication in the Command Line Interface (CLI).
 * It controls the process from requesting the password to verifying the user's input.
 */
typedef enum
{
    FSM_LOG_IN = 0,  // Log-in (waiting for password input)
    FSM_LOG_OUT = 1, // Log-out (successful authentication)
    FSM_INPUT = 2,   // Input password
    FSM_PROCESS = 3, // Processing entered password
    FSM_ERR = 4,     // Error (incorrect password)
} FSMAuthState_e;

/**
 * @brief Structure representing the CLI instance.
 *
 * This structure holds the necessary data for handling CLI operations.
 */
typedef struct
{
    struct usart_async_descriptor *uart; // UART descriptor for asynchronous communication
    struct io_descriptor *io;            // Descriptor for UART communication
    TaskHandle_t taskHandle;             // FreeRTOS task handle for the CLI task
    QueueHandle_t rxQueue;               // Queue for receiving data from UART
    QueueHandle_t txQueue;               // Queue for transmitting data to UART
    char rxBuffer[CLI_RX_BUFFER_SIZE];   // Buffer for storing received data
    char txBuffer[CLI_TX_BUFFER_SIZE];   // Buffer for storing data to be transmitted
    uint16_t rxIndex;                    // Index for tracking position in the receive buffer
    char rxChar;                         // Variable to store received character
    char txChar;                         // Variable to store transmitted character
    FSMAuthState_e authState;            // Authentication state (used for managing user login)
} Cli_s;

//===========================================================[PUBLIC INTERFACE]============================================================================================================//

/**
 * @brief Initializes the Command Line Interface (CLI).
 *
 * This function initializes the necessary peripherals, sets up UART communication,
 * and creates the CLI task that will process incoming commands and handle the output.
 * It registers the commands and prepares the system for CLI operations.
 *
 * \param[in] none;
 * \param[out] none;
 * \return int16_t - Returns 0 on successful initialization, or a negative error code on failure.
 */
int16_t CliStartup(void);

#endif /* CLI_H */
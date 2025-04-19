/**
 * @file cli_cmd.c
 * @brief Implementation of CLI commands for FreeRTOS-based systems.
 *
 * This module defines a set of CLI commands that can be registered
 * and used in a command-line interface.
 *
 * Created: 24.03.2025
 * Author: Yauheni Bialkou
 */

//=====================================================================[ INCLUDE ]=========================================================================================================//

#include "cli_cmd.h"

//=====================================================================[ INTERNAL MACRO DEFENITIONS ]======================================================================================//

#define CLI_COMMAND_COUNT (sizeof(CliCommands) / sizeof(CliCommands[0])) // Calculate the number of commands

//=====================================================================[ INTERNAL FUNCTIONS AND OBJECTS DECLARATION ]======================================================================//

static char hello[] = "Hello world \r\n";         // Message to be printed for the "hello" command
static char version[] = "CLI Version 1.0.0 \r\n"; // Message to be printed for the "version" command

/**
 * @brief Command callback function for the "hello" command.
 *
 * \param[out] pcWriteBuffer   - Buffer where the output string is stored;
 * \param[in]  xWriteBufferLen - Maximum buffer length;
 * \param[in]  pcCommandString - Command string (unused);
 * \return     pdFALSE (indicates that the output has been fully written).
 */
static BaseType_t cliCallbackHelloCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);

/**
 * @brief Command callback function for the "version" command.
 *
 * \param[out] pcWriteBuffer   - Buffer where the output string is stored;
 * \param[in]  xWriteBufferLen - Maximum buffer length;
 * \param[in]  pcCommandString - Command string (unused;
 * \return pdFALSE (indicates that the output has been fully written).
 */
static BaseType_t cliCallbackVersionCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);

/**
 * @brief Array of CLI commands.
 *
 * This array holds all available commands that can be registered in the CLI.
 */
static CLI_Command_Definition_t CliCommands[] =
    {
        {
            .pcCommand = "hello",
            .pcHelpString = "hello - prints Hello \r\n",
            .pxCommandInterpreter = cliCallbackHelloCommand,
            .cExpectedNumberOfParameters = 0,
        },
        {
            .pcCommand = "version",
            .pcHelpString = "version - prints CLI version \r\n",
            .pxCommandInterpreter = cliCallbackVersionCommand,
            .cExpectedNumberOfParameters = 0,
        }};

//=======================================================================[PUBLIC INTERFACE FUNCTIONS]======================================================================================//

/**
 * @brief Initializes CLI commands.
 *
 * \param[in]  - None;
 * \param[out] - None;
 * \return     - 0 on success, negative value on error.
 */
int16_t CliCmdInit(void)
{
    /* Loop through all commands */
    for (size_t ind = 0; ind < CLI_COMMAND_COUNT; ind++)
    {
        FreeRTOS_CLIRegisterCommand(&CliCommands[ind]);
    }
    return 0;
}

//=====================================================================[ PRIVATE FUNCTIONS ]===============================================================================================//

/**
 * @brief Command callback function for the "hello" command.
 *
 * \param[out] pcWriteBuffer   - Buffer where the output string is stored;
 * \param[in]  xWriteBufferLen - Maximum buffer length;
 * \param[in]  pcCommandString - Command string (unused);
 * \return     pdFALSE (indicates that the output has been fully written).
 */
static BaseType_t cliCallbackHelloCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    if ((pcWriteBuffer == NULL) ||
        (xWriteBufferLen == 0))
    {
        return pdFALSE;
    }

    if (strlen(hello) > xWriteBufferLen)
    {
        return pdFALSE;
    }

    strcpy(pcWriteBuffer, hello);
    return pdFALSE;
}

/**
 * @brief Command callback function for the "version" command.
 *
 * \param[out] pcWriteBuffer   - Buffer where the output string is stored;
 * \param[in]  xWriteBufferLen - Maximum buffer length;
 * \param[in]  pcCommandString - Command string (unused;
 * \return pdFALSE (indicates that the output has been fully written).
 */
static BaseType_t cliCallbackVersionCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    if ((pcWriteBuffer == NULL) ||
        (xWriteBufferLen == 0))
    {
        return pdFALSE;
    }

    if (strlen(version) > xWriteBufferLen)
    {
        return pdFALSE;
    }

    strcpy(pcWriteBuffer, version);
    return pdFALSE;
}

/**
 * @file cli_cmd.h
 * @brief Header file for CLI command definitions for FreeRTOS-based systems.
 *
 * This file declares the interface for initializing CLI commands
 * that are implemented in cli_cmd.c. These commands can be registered
 * with the FreeRTOS CLI system to provide user interaction over UART.
 *
 * Created: 28.03.2025
 * Author: Yauheni Bialkou
 */

 #ifndef CLI_CMD_H_
 #define CLI_CMD_H_
 
 //=====================================================================[ INCLUDE ]=========================================================================================================//
 
 #include <stdio.h>
 #include <string.h>
 #include "FreeRTOS.h"
 #include "FreeRTOS_CLI.h"
 
 //=====================================================================[ PUBLIC FUNCTION DECLARATIONS ]====================================================================================//
 
 /**
  * @brief Initializes CLI commands.
  *
  * Registers all predefined CLI commands with the FreeRTOS CLI system.
  *
  * \return 0 on success, negative value on error.
  */
 int16_t CliCmdInit(void);
 
 #endif /* CLI_CMD_H_ */
 
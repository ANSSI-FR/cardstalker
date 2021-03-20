/**
 * \file bridge.h
 * \copyright This file is part of the Card-Stalker project and is distributed under the GPLv3 license. See LICENSE file in the root directory of the project.
 */


#ifndef __BRIDGE_H__
#define __BRIDGE_H__


#include <stdint.h>


/**
  * \def BRIDGE_DEFAULT_COMPUTER_BAUDRATE
  * The default baudrate value used for the serial communication between the bridge and the computer.
  */
#define BRIDGE_DEFAULT_COMPUTER_BAUDRATE           9600


/**
  * \def BRIDGE_DEFAULT_COMPUTER_USART_INSTANCE
  * The default STM32 USART instance to be used for the serial communication between the bridge and the computer.
  * Note : USART2 is currently reserved for the smartcard interface.
  */
#define BRIDGE_DEFAULT_COMPUTER_USART_INSTANCE    USART1


/**
  * \def BRIDGE_DEFAULT_RECEIVE_SEND_TIMEOUT
  * The default value in milliseconds to be used as a timeout value for the receive and send operations between the bridge and the card.
  */
#define BRIDGE_DEFAULT_RECEIVE_SEND_TIMEOUT       100



/**
 * \enum BRIDGE_Status
 * This type is used to encode the returned execution code of all the functions interacting with the bridge.
 * For each function, this code indicates if the function behaved as expected or not.
 */
typedef enum BRIDGE_Status BRIDGE_Status;
enum BRIDGE_Status{
	BRIDGE_OK                       = (uint32_t)(0x00000001),
	BRIDGE_NO                       = (uint32_t)(0x00000002),
	BRIDGE_ERR                      = (uint32_t)(0x00000000)
};




BRIDGE_Status BRIDGE_InitWithDefaults(void);
BRIDGE_Status BRIDGE_Run(void);



#endif

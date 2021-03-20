/**
 * \file bridge_advanced.h
 * \copyright This file is part of the Card-Stalker project and is distributed under the GPLv3 license. See LICENSE file in the root directory of the project.
 */


#ifndef __BRIDGE_ADVANCED_H__
#define __BRIDGE_ADVANCED_H__


#include <stdint.h>
#include "reader_lib.h"
#include "bytes_buffer.h"
#include "semaphore.h"
#include "state_machine.h"


/**
  * \def BRIDGE2_DEFAULT_RECEIVE_SEND_TIMEOUT
  * The default value in milliseconds to be used as a timeout value for the receive and send operations between the bridge and the card.
  */
#define BRIDGE2_DEFAULT_RECEIVE_SEND_TIMEOUT       100

/**
  * \def BRIDGE2_DEFAULT_COMPUTER_BAUDRATE
  * The default baudrate value used for the serial communication between the bridge and the computer.
  */
#define BRIDGE2_DEFAULT_COMPUTER_BAUDRATE           9600


/**
 * \enum BRIDGE2_Status
 * This type is used to encode the returned execution code of all the functions interacting with the bridge.
 * For each function, this return code indicates if the function behaved as expected or not.
 */
typedef enum BRIDGE2_Status BRIDGE2_Status;
enum BRIDGE2_Status{
	BRIDGE2_OK                       = (uint32_t)(0x00000001),
	BRIDGE2_NO                       = (uint32_t)(0x00000002),
	BRIDGE2_EMPTY                    = (uint32_t)(0x00000003),
	BRIDGE2_ERR                      = (uint32_t)(0x00000000)
};


/**
 * \enum BRIDGE2_State
 */
typedef enum BRIDGE2_State BRIDGE2_State;
enum BRIDGE2_State{
	BRIDGE2_IDLE                     = (uint32_t)(0x00000000),
	BRIDGE2_RUNNING                  = (uint32_t)(0x00000001),
	BRIDGE2_STOP_REQUESTED           = (uint32_t)(0x00000002)
};


/**
 * \struct BRIDGE2_Handle
 * 
 */
typedef struct BRIDGE2_Handle BRIDGE2_Handle;
struct BRIDGE2_Handle{
	READER_HAL_CommSettings *pCommSettings;                     /*!<  */
	BRIDGE2_State state;                                        /*!<  */
	SEM_Handle processBusyMutex;                                /*!< Mutex used to lock the context when we are already processing a received block. */
	BUFF_Buffer cardRcvdBytes;                                  /*!< Buffer used to store the data received from the card. */
	BUFF_Buffer computerRcvdBytes;                              /*!< Buffer used to store the data received from the computer. */
	uint32_t flagDataBlockReceived;                             /*!< Flag used to indicate that a data block has been received. If 0 no data block received.  */
	uint32_t flagCtrlBlockReceived;                             /*!< Flag used to indicate that a control block has been received. If 0 no data block received.  */
	uint32_t flagAckExpected;                                   /*!< Flag used to indicate that we are waiting for an ACK block after having sent the data back to the computer. */
	uint32_t flagAckReceived;                                   /*!< Flag used to indicate that we have received the ACK from the computer (after having sent the data back to the computer). */
	SM_CtrlBlockType rcvdBlockType;                             /*!< Type of the last received Block.  */
};


/* Public functions definitions ... */
BRIDGE2_Status BRIDGE2_Init(READER_HAL_CommSettings *pCommSettings);
BRIDGE2_Status BRIDGE2_Run(void);
BRIDGE2_Status BRIDGE2_Stop(void);

BRIDGE2_Status BRIDGE2_ProcessTimerInterrupt(void);
BRIDGE2_Status BRIDGE2_EnableTimerInterrupt_Callback(void);
BRIDGE2_Status BRIDGE2_DisableTimerInterrupt_Callback(void);

BRIDGE2_Status BRIDGE2_ProcessTxeInterrupt(uint8_t *pByteToSend);
BRIDGE2_Status BRIDGE2_EnableTxeInterrupt_Callback(void);
BRIDGE2_Status BRIDGE2_DisableTxeInterrupt_Callback(void);

BRIDGE2_Status BRIDGE2_ProcessRxneInterrupt(uint8_t rcvdByte);
BRIDGE2_Status BRIDGE2_EnableRxneInterrupt_Callback(void);
BRIDGE2_Status BRIDGE2_DisableRxneInterrupt_Callback(void);


BRIDGE2_Status BRIDGE2_Sleep_Callback(void);


#endif

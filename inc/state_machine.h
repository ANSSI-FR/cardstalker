/**
 * \file state_machine.h
 * \copyright This file is part of the Card-Stalker project and is distributed under the GPLv3 license. See LICENSE file in the root directory of the project.
 * This file provides the necessary definitions for running the state machine of the communication protocol between the computer (fuzzer) and the bridge.
 */


#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__


#include <stdint.h>
#include "bytes_buffer.h"
#include "semaphore.h"



/**
 * \enum SM_Status
 * This type is used to encode the returned execution code of all the functions interacting with the state machine.
 * For each function, this return code indicates if the function behaved as expected or not.
 */
typedef enum SM_Status SM_Status;
enum SM_Status{
	SM_OK                       = (uint32_t)(0x00000001),      /*!< This code is returned to indicate that a function terminated without any errors.           */
	SM_NO                       = (uint32_t)(0x00000002),
	SM_EMPTY                    = (uint32_t)(0x00000003),      /*!< This code is returned by #SM_EvolveStateOnByteTransmission() function when it returned the last byte to send.           */
	SM_BUSY                     = (uint32_t)(0x00000004),      /*!< This code is returned by some functions to indicated that the state machine is currently to ready toprocess the given information.           */
	SM_ERR                      = (uint32_t)(0x00000000)       /*!< This code is returned to indicate that an error occured during the processing of the function.           */
};


/**
 * \enum SM_RcvState
 * This type is used to encode the current state of the reception state machine.
 */
typedef enum SM_RcvState SM_RcvState;
enum SM_RcvState{
	SM_RCVSTATE_INIT                     = (uint32_t)(0x00000000),       /*!< Initial state, waiting for control block.           */
	SM_RCVSTATE_CTRL_BYTE                = (uint32_t)(0x00000001),       /*!< State when receiving the control byte.              */
	SM_RCVSTATE_LEN_BYTE1                = (uint32_t)(0x00000002),       /*!< State when receiving len byte 1.                    */
	SM_RCVSTATE_LEN_BYTE2                = (uint32_t)(0x00000003),       /*!< State when receiving len byte 2.                    */
	SM_RCVSTATE_LEN_BYTE3                = (uint32_t)(0x00000004),       /*!< State when receiving len byte 3.                    */
	SM_RCVSTATE_DATA                     = (uint32_t)(0x00000005),       /*!< State when receiving data/payload bytes.            */
	SM_RCVSTATE_TRANSMITTED_ACK          = (uint32_t)(0x00000006),       /*!< State where we go when the transmission state machine has sent the ACK.    */
	SM_RCVSTATE_ACK_CTRL_BYTE            = (uint32_t)(0x00000007),       /*!< State where we receive the control block of the ACK we are receiving the for transmission state machine.   */
	SM_RCVSTATE_ACK_CHECK                = (uint32_t)(0x00000008),       /*!< State where we receive the check byte of the ACK we are receiving the for transmission state machine.          */
	SM_RCVSTATE_CHECK                    = (uint32_t)(0x00000009)        /*!< State when receiving CEHCK byte of the current block                     */
};


/**
 * \enum SM_SendState
 * This type is used to encode the current state of the transmission state machine.
 */
typedef enum SM_SendState SM_SendState;
enum SM_SendState{
	SM_SENDSTATE_INIT                    = (uint32_t)(0x00000000),       /*!< Initial state, sending for control block.           */
	SM_SENDSTATE_CTRL_BYTE               = (uint32_t)(0x00000001),       /*!< State when sending the control byte.                */
	SM_SENDSTATE_LEN_BYTE1               = (uint32_t)(0x00000002),       /*!< State when sending len byte 1.                      */
	SM_SENDSTATE_LEN_BYTE2               = (uint32_t)(0x00000003),       /*!< State when sending len byte 2.                      */
	SM_SENDSTATE_LEN_BYTE3               = (uint32_t)(0x00000004),       /*!< State when sending len byte 3.                      */
	SM_SENDSTATE_DATA                    = (uint32_t)(0x00000005),       /*!< State when sending data/payload bytes.              */
	SM_SENDSTATE_RCVD_ACK                = (uint32_t)(0x00000006),       /*!< State where we go when the ACK has been received by the reception state machine      */
	SM_SENDSTATE_ACK_CTRL_BYTE           = (uint32_t)(0x00000007),       /*!< State where we transmit the control byte of the ACK we are transmitting for the reception state machine.      */
	SM_SENDSTATE_ACK_CHECK               = (uint32_t)(0x00000008),       /*!< State where we transmit the check byte of the ACK we are transmitting for the reception state machine.      */
	SM_SENDSTATE_CHECK                   = (uint32_t)(0x00000009)        /*!< State when sending CHECK byte of the current block.                        */
};


/**
 * \enum SM_CtrlBlockType
 * This type is used to encode the kind of a block of bytes.
 */
typedef enum SM_CtrlBlockType SM_CtrlBlockType;
enum SM_CtrlBlockType{
	SM_DATA_BLOCK                      = (uint8_t)(0x00),
	SM_UNKNOWN_BLOCK                   = (uint8_t)(0x01),
	SM_COLD_RST_BLOCK                  = (uint8_t)(0x02),
	SM_WARM_RST_BLOCK                  = (uint8_t)(0x03),
	SM_BUSY_BLOCK                      = (uint8_t)(0x04),
	SM_ACK_BLOCK                       = (uint8_t)(0x05),
	SM_NACK_BLOCK                      = (uint8_t)(0x06)
};


/**
 * \struct SM_RcvHandle
 * This structure contains all the informations for running one instance of the communication protocol state machine in reception mode. 
 * Especially the current state.
 */
typedef struct SM_RcvHandle SM_RcvHandle;
struct SM_RcvHandle{
	SM_RcvState currentState;               /*!< Storing the current state of the communication protocol state machine.                                */
	BUFF_Buffer *pBuffer;                   /*!< Pointer on a cyclic static buffer structure (defined in bytes_buffer.h) in order to store the received bytes.      */
	uint32_t nbDataExpected;                /*!< Number of data bytes expected to be received for the current block of informations. This information is fed with the LEN field of teh current block. */
	uint32_t nbDataRcvd;                    /*!< Number of data bytes currently received for the current block.                                        */
	SM_CtrlBlockType currentBlockType;      /*!< Type of the block being currently received by the state machine.                                      */
	SEM_Handle contextAccessMutex;          /*!< Mutex object used to protect the access to the reception state machine communication context.         */
	SEM_Handle rcptProcessMutex;            /*!< Mutex object used to notify other processes that a block reception process is ongoing.                 */
	uint32_t flagAckExpected;               /*!< Flag used to indicate whether an ACK is expected from the computer. If 0, no ACK is expected to be received. Any other value, ACK from computer is expected.  */
	uint32_t flagAckTransmitted;            /*!< This is a flag used by the transmission state machine to notify the reception state machine that an ACK block has been transmitted. If value is 0, it means that no ACK has been transmitted. Any other value indicates that an ACK block has been transmitted by the reception state machine.  */
	uint32_t flagRcptOngoing;               /*!< This flag is used to indicate that a reception process in ongoing (ie : a reception process has been initiated with the #SM_ReceiveBlock() function). This flag is only relevant and defined after a call to #SM_Init() function. If value is 0, no reception has been initiated. For any other value a reception process in ongoing. */
	uint32_t flagAckRcptOccurred;           /*!< This flag is used to indicate that a ACK block reception occured during the reception process. If 0, not ACK reception occured during the reception process. If any other value, this has occurred. */
};


/**
 * \struct SM_SendHandle
 * 
 * This structure is used to save the current context of the transmission state machine.
 */
typedef struct SM_SendHandle SM_SendHandle;
struct SM_SendHandle{
	SM_SendState currentState;                  /*!< Storing the current state of the communication protocol state machine.                                */
	BUFF_Buffer *pBuffer;                       /*!< Pointer on a FIFO type circular buffer containing the bytes to be sent by the transmission state machine.          */
	SEM_Handle contextAccessMutex;              /*!< Mutex object (#SEM_Handle) used to protect the access to the transmission state machine communication context.         */
	SEM_Handle sendProcessMutex;                /*!< Mutex object (#SEM_Handle) used to notify other processes that a block transmission process is ongoing.                 */
	SM_CtrlBlockType currentBlockType;          /*!< Type of the block being currently being processed by the state machine.                                      */
	uint32_t flagAckReceived;                   /*!< This is a flag used by the reception state machine to notify the transmission state machine that an ACK block has been received. If value is 0, it means that no ACK has been received. Any other value indicates that an ACK block has been received by the reception state machine.     */
	uint32_t flagAckExpected;                   /*!< Flag used to indicate whether an ACK is expected to be sent to the computer. If 0, no ACK is expected to be sent. Any other value, sending ACK to the computer is expected.     */
	uint32_t flagEmpty;                         /*!< Flag used to indicate that the transmission process is over (ie: we went through all the steps, there is nothing more to to until a new process is started). If flag is 0, the process is not over. Any other value, the process is done.     */
	uint32_t flagSendOngoing;                   /*!< This flag is used to indicate that a transmission process in ongoing (ie : a transmission process has been initiated with the #SM_SendBlock() function). This flag is only relevant and defined after a call to #SM_Init() function. If value is 0, no transmission has been initiated. For any other value a transmission process in ongoing. */
};


/**
 * \struct SM_Handle
 * The state machine dealing with the USART connection with the computer is approximately separated into two distinct state machines.
 * One is for handling the reception process from the computerand the other one is for dealing with the transmission process.
 * This structure aims to provide a single communication context grouping the context of te two staemachines aformentionned.
 */
typedef struct SM_Handle SM_Handle;
struct SM_Handle{
	SM_RcvHandle rcvHandle;                       /*!<Reception state machine communication context.     */
	SM_SendHandle sendHandle;                     /*!<Transmission state machine communication context.  */
};



SM_Status SM_Init(SM_Handle *pHandle);

/*  Public functions definitions for reception ...  */
SM_Status SM_ReceiveBlock(SM_Handle *pHandle, BUFF_Buffer *pBuffer);
SM_Status SM_EvolveStateOnByteReception(SM_Handle *pHandle, uint8_t rcvdByte);
SM_Status SM_GetRcptBufferPtr(SM_Handle *pHandle, BUFF_Buffer **ppBuffer);
SM_Status SM_IsDataAvail(SM_Handle *pHandle);
SM_Status SM_IsAllDataRecieved(SM_Handle *pHandle);

SM_Status SM_BlockRecievedCallback(SM_Handle *pHandle);
SM_Status SM_CtrlBlockRecievedCallback(SM_Handle *pHandle);
SM_Status SM_DataBlockRecievedCallback(SM_Handle *pHandle);
SM_Status SM_COLD_RST_BLOCK_Callback(SM_Handle *pHandle);
SM_Status SM_WARM_RST_BLOCK_Callback(SM_Handle *pHandle);
SM_Status SM_UNKNOWN_BLOCK_Callback(SM_Handle *pHandle);
SM_Status SM_ACK_BLOCK_ReceivedCallback(SM_Handle *pHandle);

SM_Status SM_EnableRxneInterrupt_Callback(SM_Handle *pHandle);
SM_Status SM_DisableRxneInterrupt_Callback(SM_Handle *pHandle);


/* Public functions definitions for transmission */
SM_Status SM_SendBlock(SM_Handle *pHandle, BUFF_Buffer *pBuffer, SM_CtrlBlockType type);
SM_Status SM_EvolveStateOnByteTransmission(SM_Handle *pHandle, uint8_t *pByteToSend);
SM_Status SM_GetSendBufferPtr(SM_Handle *pHandle, BUFF_Buffer **ppBuffer);

SM_Status SM_BlockSentCallback(SM_Handle *pHandle);
SM_Status SM_ACK_BLOCK_TransmittedCallback(SM_Handle *pHandle);

SM_Status SM_EnableTxeInterrupt_Callback(SM_Handle *pHandle);
SM_Status SM_DisableTxeInterrupt_Callback(SM_Handle *pHandle);


#endif

/**
 * \file bridge_advanced.c
 * \copyright This file is part of the Card-Stalker project and is distributed under the GPLv3 license. See LICENSE file in the root directory of the project.
 * This file provides a top-level public API for enabling the user to initialize, operate and run the computer-to-card bridge.
 * 
 * This file is the most top-level one, thus aggregating all the components provided by all the other files.
 * From a functional point of view, the code contained in this file coordinates two communication interfaces :
 *   1) The first one is the serial interface between the computer/fuzzer and the bridge.
 *   2) The other one is the ISO/IEC7816-3 interface between the smartcard and the bridge.
 * 
 * The code of the bridge is designed to be full asynchronous and full duplex from the USART side, thus the code in mainly relying on interruption mechanisms.
 * From the reader side (smartcard side) it is synchronous and half-duplex.
 */


#include "bridge_advanced.h"
#include "reader_lib.h"
#include "bytes_buffer.h"
#include "state_machine.h"
#include "semaphore.h"



/* Global variables declarations ...  */

/**
 * \var static SM_Handle globalUsartHandle
 * globalUsartHandle is a global data structure (of SM_Handle type) local to this file.
 * It stores the communication context of the serial communication acoss the bridge and the computer.
 */
static SM_Handle globalUsartHandle;

/**
 * \var static BRIDGE2_Handle globalBridgeHandle
 * globalBridgeHandle is a data structure (of BRIDGE2_Handle type) storing the current state and parameters of the bridge.
 */
static BRIDGE2_Handle globalBridgeHandle;  /* TODO: Put USART context into bridge context ??  */



/* Private functions definitions (functions local to this file) ...  */
static BRIDGE2_Status BRIDGE2_SendBufferToCard(BUFF_Buffer *pBuffer);
static BRIDGE2_Status BRIDGE2_RcvBufferFromCard(BUFF_Buffer *pBuffer);
static BRIDGE2_Status BRIDGE2_ApplyRcvdDataBlock(void);
static BRIDGE2_Status BRIDGE2_ApplyRcvdCtrlBlock(void);
static BRIDGE2_Status BRIDGE2_ApplyColdReset(void);
static BRIDGE2_Status BRIDGE2_StartNewReception(void);



/* Public functions declarations ...  */

/**
 * \fn BRIDGE2_Status BRIDGE2_Init(READER_HAL_CommSettings *pCommSettings)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * \param *pCommSettings is a pointer on a READER_HAL_CommSettings data structure. the content of the structure is not going to be modifed. It should have been initialized before calling this function. It can be done thanks to the methods provided with the iso7816 reader librairy, for example by calling the READER_HAL_InitWithDefaults() function. It has to be pointing on a valid low level communication context for the reader. 
 * This function initializes the bridge. It resets flags, values and parameters in the bridge context data structure.
 * Warning : this function operates on the globalBridgeHandle global data structure which is local to this file.
 */
BRIDGE2_Status BRIDGE2_Init(READER_HAL_CommSettings *pCommSettings){
	SM_Status smRv;
	BRIDGE2_Status rv;
	SEM_Status mutexRv;
	
	
	mutexRv = SEM_Init(&(globalBridgeHandle.processBusyMutex), 1);
	if(mutexRv != SEM_OK) return BRIDGE2_ERR;
	
	rv = BRIDGE2_DisableTimerInterrupt_Callback();
	if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	
	globalBridgeHandle.pCommSettings = pCommSettings;
	globalBridgeHandle.state = BRIDGE2_IDLE;
	globalBridgeHandle.flagDataBlockReceived = 0;
	globalBridgeHandle.flagCtrlBlockReceived = 0;
	globalBridgeHandle.flagAckExpected = 0;
	globalBridgeHandle.flagAckReceived = 0;
	
	smRv = SM_Init(&globalUsartHandle);
	if(smRv != SM_OK) return BRIDGE2_ERR;
	
	
	return BRIDGE2_OK;
}


/**
 * \fn BRIDGE2_Status BRIDGE2_Run
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * This function starts the bridge. As the bridge operates asychronously, this function is non-blocking.
 * Warning : this function operates on the globalBridgeHandle global data structure which is local to this file.
 */
BRIDGE2_Status BRIDGE2_Run(void){
	BRIDGE2_Status rv;
	
	
	globalBridgeHandle.state = BRIDGE2_RUNNING;
	
	rv = BRIDGE2_StartNewReception();
	if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	
	rv = BRIDGE2_EnableTimerInterrupt_Callback();
	if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	
	rv = BRIDGE2_EnableRxneInterrupt_Callback();
	if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	
	
	return BRIDGE2_OK;
}


/**
 * \fn BRIDGE2_Status BRIDGE2_Stop(void)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * This function stops the bridge after the ongoing operations are finished. This function is non-blocking.
 * Warning : this function operates on the globalBridgeHandle global data structure which is local to this file.
 */
BRIDGE2_Status BRIDGE2_Stop(void){
	globalBridgeHandle.state = BRIDGE2_STOP_REQUESTED;
	
	
	return BRIDGE2_OK;
}


/**
 * \fn BRIDGE2_Status BRIDGE2_ProcessTimerInterrupt(void)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * This function is designed to be called from a periodic timer interrupt routine. It is aimed to periodically check for events/data reception from the usart interface.
 * When implementing the bridge for a specific target, the developper is in charge of configuring the timer interruption and to place a call to this function inside.
 * There is no strong requisites about the period of the interruption, few miliseconds are fine for example.
 * Warning : this function operates on the globalBridgeHandle global data structure which is local to this file.
 */
BRIDGE2_Status BRIDGE2_ProcessTimerInterrupt(void){
	BRIDGE2_Status rv;
	SEM_Status mutexRv;
	
	
	mutexRv = SEM_TryLock(&(globalBridgeHandle.processBusyMutex));
	if((mutexRv != SEM_LOCKED) && (mutexRv != SEM_UNLOCKED)) return BRIDGE2_ERR;
	
	
	if(mutexRv == SEM_UNLOCKED){
		/* If we have received a data block from the computer ...  */
		if((globalBridgeHandle.flagDataBlockReceived) != 0){
			rv = BRIDGE2_ApplyRcvdDataBlock();
			if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
			
			globalBridgeHandle.flagDataBlockReceived = 0;
		}
		
		if((globalBridgeHandle.flagAckReceived) != 0){
			rv = BRIDGE2_StartNewReception();
			if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
			
			globalBridgeHandle.flagAckReceived = 0;
		}
		
		/* If we have received a control block from the computer ...  */
		if((globalBridgeHandle.flagCtrlBlockReceived) != 0){
			rv = BRIDGE2_ApplyRcvdCtrlBlock();
			if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
			
			globalBridgeHandle.flagCtrlBlockReceived = 0;
		}
		
		mutexRv = SEM_Release(&(globalBridgeHandle.processBusyMutex));
		if(mutexRv != SEM_OK) return BRIDGE2_ERR;
	}
	
	
	rv = BRIDGE2_Sleep_Callback();
	if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	
	
	return BRIDGE2_OK;
}


/**
 * \fn __attribute__((weak)) BRIDGE2_Status BRIDGE2_EnableTimerInterrupt_Callback(void)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * The implementer of the bridge for a specific target has to make its own implementation of this function because its code might be hardware dependent.
 * The implementation of this function has to enable the periodic timer interruption.
 * The developper has to make sure to return a BRIDGE2_OK execution code if the interruption is correctly activated.
 */
__attribute__((weak)) BRIDGE2_Status BRIDGE2_EnableTimerInterrupt_Callback(void){
	return BRIDGE2_OK;
}


/**
 * \fn __attribute__((weak)) BRIDGE2_Status BRIDGE2_DisableTimerInterrupt_Callback(void)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * The implementer of the bridge for a specific target has to make its own implementation of this function because its code might be hardware dependent.
 * The implementation of this function has to disable the periodic timer interruption.
 * The developper has to make sure to return a BRIDGE2_OK execution code if the interruption is correctly deactivated.
 */
__attribute__((weak)) BRIDGE2_Status BRIDGE2_DisableTimerInterrupt_Callback(void){
	return BRIDGE2_OK;
}


/**
 * \fn BRIDGE2_Status BRIDGE2_ProcessRxneInterrupt(uint8_t rcvdByte)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * \param rcvdByte is the value of the early received byte in the UART interrupt routine.
 * This function is designed to be called from a UART byte reception interrupt routine. It is aimed to retrieve the early received byte and to evolve bridge's state consequently.
 * When implementing the bridge for a specific target, the developper is in charge of configuring the UART interruption and to place a call to this function inside.
 * Warning : this function operates on the globalUsartHandle global data structure which is local to this file.
 */
BRIDGE2_Status BRIDGE2_ProcessRxneInterrupt(uint8_t rcvdByte){
	SM_Status rv;
	
	
	rv = SM_EvolveStateOnByteReception(&globalUsartHandle, rcvdByte);
	if(rv != SM_OK) return BRIDGE2_ERR;
	
	
	return BRIDGE2_OK;
}


/**
 * \fn BRIDGE2_Status BRIDGE2_ProcessTxeInterrupt(uint8_t *pByteToSend)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * \param *pByteToSend is a pointer on an uint8_t value. The location will be filled with the next byte that has to be sent by the UART interrupt routine.
 * This function is designed to be called from a UART buffer empty interrupt routine. It is aimed to transmitthe next byte ready to be sent back to the computer and to evolve bridge's state consequently.
 * When implementing the bridge for a specific target, the developper is in charge of configuring the UART interruption and to place a call to this function inside.
 * Warning : this function operates on the globalUsartHandle global data structure which is local to this file.
 */
BRIDGE2_Status BRIDGE2_ProcessTxeInterrupt(uint8_t *pByteToSend){
	SM_Status smRv;
	BRIDGE2_Status rv;
	
	
	smRv = SM_EvolveStateOnByteTransmission(&globalUsartHandle, pByteToSend);
	if((smRv != SM_OK) && (smRv != SM_EMPTY)) return BRIDGE2_ERR;
	
	if(smRv == SM_EMPTY){
		rv = BRIDGE2_DisableTxeInterrupt_Callback();
		if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
		
		return BRIDGE2_EMPTY;
	}
	
	
	return BRIDGE2_OK;
}


/**
 * \fn __attribute__((weak)) BRIDGE2_Status BRIDGE2_EnableTxeInterrupt_Callback(void)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * The implementer of the bridge for a specific target has to make its own implementation of this function because its code might be hardware dependent.
 * The implementation of this function has to enable the TX Empty interruption of the UART peripheral.
 * The developper has to make sure to return a BRIDGE2_OK execution code if the interruption is correctly activated.
 */
__attribute__((weak)) BRIDGE2_Status BRIDGE2_EnableTxeInterrupt_Callback(void){
	return BRIDGE2_OK;
}


/**
 * \fn __attribute__((weak)) BRIDGE2_Status BRIDGE2_DisableTxeInterrupt_Callback(void)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * The implementer of the bridge for a specific target has to make its own implementation of this function because its code might be hardware dependent.
 * The implementation of this function has to disable the TX Empty interruption of the UART peripheral.
 * The developper has to make sure to return a BRIDGE2_OK execution code if the interruption is correctly activated.
 */
__attribute__((weak)) BRIDGE2_Status BRIDGE2_DisableTxeInterrupt_Callback(void){
	return BRIDGE2_OK;
}


/**
 * \fn __attribute__((weak)) BRIDGE2_Status BRIDGE2_EnableTxeInterrupt_Callback(void)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * The implementer of the bridge for a specific target has to make its own implementation of this function because its code might be hardware dependent.
 * The implementation of this function has to enable the RX Not Empty interruption of the UART peripheral.
 * The developper has to make sure to return a BRIDGE2_OK execution code if the interruption is correctly activated.
 */
__attribute__((weak)) BRIDGE2_Status BRIDGE2_EnableRxneInterrupt_Callback(void){
	return BRIDGE2_OK;
}


/**
 * \fn __attribute__((weak)) BRIDGE2_Status BRIDGE2_DisableRxneInterrupt_Callback(void)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * The implementer of the bridge for a specific target has to make its own implementation of this function because its code might be hardware dependent.
 * The implementation of this function has to diable the RX Not Empty interruption of the UART peripheral.
 * The developper has to make sure to return a BRIDGE2_OK execution code if the interruption is correctly activated.
 */
__attribute__((weak)) BRIDGE2_Status BRIDGE2_DisableRxneInterrupt_Callback(void){
	return BRIDGE2_OK;
}


__attribute__((weak)) BRIDGE2_Status BRIDGE2_Sleep_Callback(void){
	return BRIDGE2_OK;
}



/* Private functions declarations ...  */

/**
 * \fn static BRIDGE2_Status BRIDGE2_SendBufferToCard(BUFF_Buffer *pBuffer)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * \param *pBuffer is a pointer to a BUFF_Buffer data structure containing the bytes of data to be sent to the smartcard on the I/O half-duplex transmission line.
 * This function transmists a buffer of bytes to the smartcard by making use of the iso7816 reader librairy.
 */
static BRIDGE2_Status BRIDGE2_SendBufferToCard(BUFF_Buffer *pBuffer){
	BUFF_Status buffRv;
	READER_Status readerRv;
	READER_HAL_CommSettings *pSettings;
	uint8_t byte;
	
	
	pSettings = globalBridgeHandle.pCommSettings;
	
	while(BUFF_IsEmpty(pBuffer) == BUFF_NO){
		buffRv = BUFF_Dequeue(pBuffer, &byte);
		if(buffRv != BUFF_OK) return BRIDGE2_ERR;
		
		readerRv = READER_HAL_SendChar(pSettings, READER_HAL_PROTOCOL_T1, byte, BRIDGE2_DEFAULT_RECEIVE_SEND_TIMEOUT);
		if(readerRv != READER_OK) return BRIDGE2_ERR;
	}
	
	
	return BRIDGE2_OK;
}


/**
 * \fn static BRIDGE2_Status BRIDGE2_RcvBufferFromCard(BUFF_Buffer *pBuffer)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * \param *pBuffer is a pointer to a BUFF_Buffer data structure where the received bytes (from the smartcard) are going to be placed in. The buffer will be reset by this function before putting the bytes in.
 * This function receives characters from the smartcard on the I/O transmission line. It stops when timeout or when the buffer overflows.
 */
static BRIDGE2_Status BRIDGE2_RcvBufferFromCard(BUFF_Buffer *pBuffer){
	BUFF_Status buffRv;
	READER_Status readerRv;
	READER_HAL_CommSettings *pSettings;
	uint8_t byte;
	
	
	pSettings = globalBridgeHandle.pCommSettings;
	
	buffRv = BUFF_Init(pBuffer);
	if(buffRv != BUFF_OK) return BRIDGE2_ERR;
	
	do{
		readerRv = READER_HAL_RcvChar(pSettings, READER_HAL_PROTOCOL_T1, &byte, BRIDGE2_DEFAULT_RECEIVE_SEND_TIMEOUT);
		if((readerRv != READER_OK) && (readerRv != READER_TIMEOUT)) return BRIDGE2_ERR;
		
		if(readerRv != READER_TIMEOUT){
			buffRv = BUFF_Enqueue(pBuffer, byte);
			if(buffRv != BUFF_OK) return BRIDGE2_ERR;
		}
		
	}while(readerRv != READER_TIMEOUT);
	
	
	return BRIDGE2_OK;
}


/**
 * \fn static BRIDGE2_Status BRIDGE2_ApplyColdReset(void)
 * \return BRIDGE2_Status execution code. BRIDGE2_OK indicates nominal execution of the function.
 * This function applies a cold reset procedure to the smartcard. See ISO/IEC7816-3 section 6.2.2.
 */
static BRIDGE2_Status BRIDGE2_ApplyColdReset(void){
	READER_Status readerRv;
	
	
	readerRv = READER_HAL_DoColdReset();
	if(readerRv != READER_OK) return BRIDGE2_ERR;
	
	return BRIDGE2_OK;
}


static BRIDGE2_Status BRIDGE2_ApplyRcvdDataBlock(void){
	BRIDGE2_Status rv;
	READER_Status status;
	READER_HAL_CommSettings *pSettings;
	SM_Status smRv;
	
	
	pSettings = globalBridgeHandle.pCommSettings;
	
	/* We send to the card the previously received data from the computer ...  */
	rv = BRIDGE2_SendBufferToCard(&(globalBridgeHandle.computerRcvdBytes));
	if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	
	status = READER_HAL_WaitUntilSendComplete(pSettings);
	if(status != READER_OK) return BRIDGE2_ERR;
	
	/* We get back the answer from the card in a temporary buffer ... */
	rv = BRIDGE2_RcvBufferFromCard(&(globalBridgeHandle.cardRcvdBytes));
	if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	
	/* We send this data back to the computer inside a block ...  */
	do{	
		smRv = SM_SendBlock(&globalUsartHandle, &(globalBridgeHandle.cardRcvdBytes), SM_DATA_BLOCK);
		if((smRv != SM_OK) && (smRv != SM_BUSY)) return BRIDGE2_ERR;
	}while(smRv == SM_BUSY);   /* TODO : Adding a sleep function ?? ...  */
	
	rv = BRIDGE2_EnableTxeInterrupt_Callback();  /* TODO : Remove this line (dead code ??) */
	if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	
	globalBridgeHandle.flagAckExpected = 1;
	
	
	return BRIDGE2_OK;
}


static BRIDGE2_Status BRIDGE2_StartNewReception(void){
	BRIDGE2_Status rv;
	SM_Status smRv;
	
	
	/* We check if we have to start another block reception from the computer ...  */
	if((globalBridgeHandle.state) == BRIDGE2_RUNNING){
		do{
			smRv = SM_ReceiveBlock(&globalUsartHandle, &(globalBridgeHandle.computerRcvdBytes));   /* TODO : Adding a sleep function ?? ...  */
			if((smRv != SM_OK) && (smRv != SM_BUSY)) return BRIDGE2_ERR;
		}while(smRv == SM_BUSY);
	}
	else{
		/* If we receive a block after the bridge has been stopped it means this is the last one ...  */
		rv = BRIDGE2_DisableTimerInterrupt_Callback();
		if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	}
	
	
	return BRIDGE2_OK;
}


static BRIDGE2_Status BRIDGE2_ApplyRcvdCtrlBlock(void){
	BRIDGE2_Status rv;
	SM_CtrlBlockType type;
	
	
	type = globalBridgeHandle.rcvdBlockType;
	
	switch(type){
		case SM_COLD_RST_BLOCK:
			rv = BRIDGE2_ApplyColdReset();
			if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
			break;
			
		default:
			break;
	}
	
	rv = BRIDGE2_StartNewReception();
	if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	
	
	return BRIDGE2_OK;
}


/* Callback functions from the asynchronous usart state machine ...  */

SM_Status SM_BlockRecievedCallback(SM_Handle *pHandle){
	BRIDGE2_Status rv;
	
	
	rv = BRIDGE2_DisableRxneInterrupt_Callback();
	if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	
	
	return SM_OK;
}


SM_Status SM_CtrlBlockRecievedCallback(SM_Handle *pHandle){
	globalBridgeHandle.flagCtrlBlockReceived = 1;
	globalBridgeHandle.rcvdBlockType = pHandle->rcvHandle.currentBlockType;
	
	
	return SM_OK;
}


SM_Status SM_DataBlockRecievedCallback(SM_Handle *pHandle){	
	globalBridgeHandle.flagDataBlockReceived = 1;
	globalBridgeHandle.rcvdBlockType = SM_DATA_BLOCK;
	
	
	return SM_OK;
}


SM_Status SM_BlockSentCallback(SM_Handle *pHandle){
	BRIDGE2_Status rv;
	
	
	rv = BRIDGE2_DisableTxeInterrupt_Callback();
	if(rv != BRIDGE2_OK) return BRIDGE2_ERR;
	
	
	return SM_OK;
}


SM_Status SM_EnableRxneInterrupt_Callback(SM_Handle *pHandle){
	BRIDGE2_Status rv;
	
	
	rv = BRIDGE2_EnableRxneInterrupt_Callback();
	if(rv != BRIDGE2_OK) return SM_ERR;
	
	return SM_OK;
}


SM_Status SM_DisableRxneInterrupt_Callback(SM_Handle *pHandle){
	BRIDGE2_Status rv;
	
	
	rv = BRIDGE2_DisableRxneInterrupt_Callback();
	if(rv != BRIDGE2_OK) return SM_ERR;
	
	return SM_OK;
}


SM_Status SM_EnableTxeInterrupt_Callback(SM_Handle *pHandle){
	BRIDGE2_Status rv;
	
	
	rv = BRIDGE2_EnableTxeInterrupt_Callback();
	if(rv != BRIDGE2_OK) return SM_ERR;
	
	return SM_OK;
}


SM_Status SM_DisableTxeInterrupt_Callback(SM_Handle *pHandle){
	BRIDGE2_Status rv;
	
	
	rv = BRIDGE2_DisableTxeInterrupt_Callback();
	if(rv != BRIDGE2_OK) return SM_ERR;
	
	return SM_OK;
}


SM_Status SM_ACK_BLOCK_ReceivedCallback(SM_Handle *pHandle){
	if((globalBridgeHandle.flagAckExpected) != 0){
		globalBridgeHandle.flagAckReceived = 1;
	}
	
	return SM_OK;
}

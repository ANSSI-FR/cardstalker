/**
 * \file state_machine.c
 * \copyright This file is part of the Card-Stalker project and is distributed under the GPLv3 license. See LICENSE file in the root directory of the project.
 * This file provides the state machine for the protocol ruling the serial communication between the bridge and the computer.
 */


#include "state_machine.h"
#include "bytes_buffer.h"
#include "semaphore.h"



/* Private functions declarations ...  */


/* Specific to reception state machine private functions ...  */
static SM_Status SM_InitRcv(SM_Handle *pHandle);

static SM_Status SM_ApplyRcvState(SM_Handle *pHandle, uint8_t rcvdByte);
static SM_Status SM_ApplyState_SM_RCVSTATE_INIT(SM_Handle *pHandle);
static SM_Status SM_ApplyState_SM_RCVSTATE_TRANSMITTED_ACK(SM_Handle *pHandle, uint8_t rcvdByte);
static SM_Status SM_ApplyState_SM_RCVSTATE_CTRL_BYTE(SM_Handle *pHandle, uint8_t rcvdByte);
static SM_Status SM_ApplyState_SM_RCVSTATE_LEN_BYTE1(SM_Handle *pHandle, uint8_t rcvdByte);
static SM_Status SM_ApplyState_SM_RCVSTATE_LEN_BYTE2(SM_Handle *pHandle, uint8_t rcvdByte);
static SM_Status SM_ApplyState_SM_RCVSTATE_LEN_BYTE3(SM_Handle *pHandle, uint8_t rcvdByte);
static SM_Status SM_ApplyState_SM_RCVSTATE_DATA(SM_Handle *pHandle, uint8_t rcvdByte);
static SM_Status SM_ApplyState_SM_RCVSTATE_CHECK(SM_Handle *pHandle, uint8_t rcvdByte);
static SM_Status SM_ApplyState_SM_RCVSTATE_ACK_CTRL_BYTE(SM_Handle *pHandle, uint8_t rcvdByte);
static SM_Status SM_ApplyState_SM_RCVSTATE_ACK_CHECK(SM_Handle *pHandle, uint8_t rcvdByte);

static SM_Status SM_ComputeNextRcvState(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_INIT(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_TRANSMITTED_ACK(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_CTRL_BYTE(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_LEN_BYTE1(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_LEN_BYTE2(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_LEN_BYTE3(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_DATA(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_CHECK(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_ACK_CTRL_BYTE(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_ACK_CHECK(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState);

static SM_Status SM_MoveToNextRcvState(SM_Handle *pHandle, SM_RcvState nextState);
static SM_Status Apply_SM_ACK_BLOCK_Received(SM_Handle *pHandle);
static SM_Status SM_EndBlockRcvProcess(SM_Handle *pHandle);
static SM_Status SM_ReceiveBlockWithSameBuffer(SM_Handle *pHandle);



/* Specific to transmission state machine private functions ...  */
static SM_Status SM_InitSend(SM_Handle *pHandle);

static SM_Status SM_ApplySendState(SM_Handle *pHandle, uint8_t *pByteToSend);
static SM_Status SM_ApplyState_SM_SENDSTATE_INIT(SM_Handle *pHandle);
static SM_Status SM_ApplyState_SM_SENDSTATE_RCVD_ACK(SM_Handle *pHandle, uint8_t *pByteToSend);
static SM_Status SM_ApplyState_SM_SENDSTATE_CTRL_BYTE(SM_Handle *pHandle, uint8_t *pByteToSend);
static SM_Status SM_ApplyState_SM_SENDSTATE_LEN_BYTE1(SM_Handle *pHandle, uint8_t *pByteToSend);
static SM_Status SM_ApplyState_SM_SENDSTATE_LEN_BYTE2(SM_Handle *pHandle, uint8_t *pByteToSend);
static SM_Status SM_ApplyState_SM_SENDSTATE_LEN_BYTE3(SM_Handle *pHandle, uint8_t *pByteToSend);
static SM_Status SM_ApplyState_SM_SENDSTATE_DATA(SM_Handle *pHandle, uint8_t *pByteToSend);
static SM_Status SM_ApplyState_SM_SENDSTATE_CHECK(SM_Handle *pHandle, uint8_t *pByteToSend);
static SM_Status SM_ApplyState_SM_SENDSTATE_ACK_CTRL_BYTE(SM_Handle *pHandle, uint8_t *pByteToSend);
static SM_Status SM_ApplyState_SM_SENDSTATE_ACK_CHECK(SM_Handle *pHandle, uint8_t *pByteToSend);

static SM_Status SM_ComputeNextSendState(SM_Handle *pHandle, SM_SendState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_INIT(SM_Handle *pHandle, SM_SendState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_RCVD_ACK(SM_Handle *pHandle, SM_SendState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_CTRL_BYTE(SM_Handle *pHandle, SM_SendState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_LEN_BYTE1(SM_Handle *pHandle, SM_SendState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_LEN_BYTE2(SM_Handle *pHandle, SM_SendState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_LEN_BYTE3(SM_Handle *pHandle, SM_SendState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_DATA(SM_Handle *pHandle, SM_SendState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_CHECK(SM_Handle *pHandle, SM_SendState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_ACK_CTRL_BYTE(SM_Handle *pHandle, SM_SendState *pNextState);
static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_ACK_CHECK(SM_Handle *pHandle, SM_SendState *pNextState);

static SM_Status SM_MoveToNextSendState(SM_Handle *pHandle, SM_SendState nextState);
static SM_Status Apply_SM_ACK_BLOCK_Transmitted(SM_Handle *pHandle);
static SM_Status SM_EndBlockSendProcess(SM_Handle *pHandle);


/* General usage private functions ....  */
static SM_Status SM_DoesThisBlockNeedAnAck(SM_CtrlBlockType type);


/* Public functions definitions ...  */


/**
 * \fn SM_Status SM_Init(SM_Handle *pHandle)
 * \brief Initializes both the reception and transmission state machine contexts.
 * \return This function returns a SM_Status execution code.
 * \param *pHandle is a pointer on a SM_Handle struct containing both the current context of the reception state machine and the current context of the transmission state machine.
 */
SM_Status SM_Init(SM_Handle *pHandle){
	SM_Status rv;
	
	
	rv = SM_InitRcv(pHandle);
	if(rv != SM_OK) return SM_ERR;
	
	rv = SM_InitSend(pHandle);
	if(rv != SM_OK) return SM_ERR;
	
	
	return SM_OK;
}


/**
 * \fn static SM_Status SM_InitRcv(SM_Handle *pHandle)
 * \brief Initializes the reception state machine context.
 * \return This function returns a SM_Status execution code.
 * \param *pHandle is a pointer on a SM_RcvHandle struct containing the current context of the reception state machine. (TODO : Update doxy)
 */
static SM_Status SM_InitRcv(SM_Handle *pHandle){
	SM_Status rv;
	SEM_Status semRv;
	SM_RcvHandle *pRcvHandle;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	
	rv = SM_DisableRxneInterrupt_Callback(pHandle);
	if(rv != SM_OK) return SM_ERR;
	
	/* Initializing the semaphore with a capacity of 1 (mutex mode) ...  */
	semRv = SEM_Init(&(pRcvHandle->contextAccessMutex), 1);
	if(semRv != SEM_OK) return SM_ERR;
	
	semRv = SEM_Init(&(pRcvHandle->rcptProcessMutex), 1);
	if(semRv != SEM_OK) return SM_ERR;
	
	
	/* We initialize the ACK expected flag ...  */
	pRcvHandle->flagAckExpected = 0;
	pRcvHandle->flagAckTransmitted = 0;
	pRcvHandle->flagRcptOngoing = 0;
	pRcvHandle->flagAckRcptOccurred = 0;
	
	return SM_OK;
}


/**
 * \fn static SM_Status SM_EndBlockRcvProcess(SM_Handle *pHandle)
 * \brief Perform all the necessary actions on the communication context when we went through all the steps to receive a block.
 * \param *pHandle Is a pointer on a #SM_Handle struct containing the current communication context.
 * \return This function returns an #SM_Status error code to indicate if the function behaved as expected or not.
 * 
 * This function typically relases the mutex indicating that a reception process in ongoing (#rcvProcessMutex) and calls all the callback functions.
 * It also eventually starts a new reception process if the transmission state machine is expecting/waiting for an ACK.
 */
static SM_Status SM_EndBlockRcvProcess(SM_Handle *pHandle){
	SM_RcvHandle *pRcvHandle;
	SM_CtrlBlockType type;
	SEM_Status mutexRv;
	SM_Status rv;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	type = pRcvHandle->currentBlockType;
	
	mutexRv = SEM_Release(&(pRcvHandle->rcptProcessMutex));
	if(mutexRv != SEM_OK) return SM_ERR;
	
	pRcvHandle->flagRcptOngoing = 0;
	
	if((pRcvHandle->flagAckRcptOccurred) != 0){
		rv = SM_ACK_BLOCK_ReceivedCallback(pHandle);
		if(rv != SM_OK) return SM_ERR;
	}
	
	switch(type){
		case SM_ACK_BLOCK:
			break;
		
		case SM_DATA_BLOCK:
			rv = SM_DataBlockRecievedCallback(pHandle);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_COLD_RST_BLOCK:
			rv = SM_CtrlBlockRecievedCallback(pHandle);
			if(rv != SM_OK) return SM_ERR;
			
			rv = SM_COLD_RST_BLOCK_Callback(pHandle);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_WARM_RST_BLOCK:
			rv = SM_CtrlBlockRecievedCallback(pHandle);
			if(rv != SM_OK) return SM_ERR;
			
			rv = SM_WARM_RST_BLOCK_Callback(pHandle);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_UNKNOWN_BLOCK:
			rv = SM_CtrlBlockRecievedCallback(pHandle);
			if(rv != SM_OK) return SM_ERR;
			
			rv = SM_UNKNOWN_BLOCK_Callback(pHandle);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		default:
			return SM_ERR;
	}
	
	rv = SM_BlockRecievedCallback(pHandle);
	if(rv != SM_OK) return SM_ERR;
	
	rv = SM_DisableRxneInterrupt_Callback(pHandle);
	if(rv != SM_OK) return SM_ERR;
	
	/* We start a new block reception if we are expecting an ACK ...  */
	if((pHandle->rcvHandle.flagAckExpected) != 0){
		rv = SM_ReceiveBlockWithSameBuffer(pHandle);
		if(rv != SM_OK) return SM_ERR;
	}
		
	
	return SM_OK;
}


static SM_Status SM_ReceiveBlockWithSameBuffer(SM_Handle *pHandle){
	SM_Status rv;
	
	
	rv = SM_ReceiveBlock(pHandle, pHandle->rcvHandle.pBuffer);
	if(rv != SM_OK) return rv;
	
	
	return SM_OK;
}


/**
 * \fn SM_Status SM_ReceiveBlock(SM_Handle *pHandle, BUFF_Buffer *pBuffer)
 * \brief This non-blocking function starts a block reception process.
 * \return This function return a #SM_Status execution code. If #SM_OK, the reception process has started correctly. If #SM_BUSY, the reception process has not been started because another reception process is ongoing. Any other value indicates an error.
 * \param *pHandle is a pointer on a SM_Handle struct containing the current context of the state machine.
 * \param *pBuffer is a pointer on a #BUFF_Buffer structure that is gonne be used to store the received data bytes if we are receiving a data block.
 */
SM_Status SM_ReceiveBlock(SM_Handle *pHandle, BUFF_Buffer *pBuffer){
	SM_RcvHandle *pRcvHandle;
	SEM_Status mutexRv;
	SM_Status rv;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	
	mutexRv = SEM_TryLock(&(pRcvHandle->rcptProcessMutex));
	if((mutexRv != SEM_UNLOCKED) && (mutexRv != SEM_LOCKED)) return SM_ERR;
	
	if(mutexRv == SEM_UNLOCKED){
		pRcvHandle->pBuffer = pBuffer;
		pRcvHandle->currentState = SM_RCVSTATE_INIT;
		pRcvHandle->currentBlockType = SM_UNKNOWN_BLOCK;
		pRcvHandle->flagRcptOngoing = 1;
		pRcvHandle->flagAckRcptOccurred = 0;
	
		rv = SM_ApplyRcvState(pHandle, 0x00);
		if(rv != SM_OK) return SM_ERR;
		
		rv = SM_EnableRxneInterrupt_Callback(pHandle);
		if(rv != SM_OK) return SM_ERR;
	}
	else{
		return SM_BUSY;
	}
	
	return SM_OK;
}


/**
 * \fn SM_Status SM_EvolveStateOnByteReception(SM_Handle *pHandle, uint8_t rcvdByte)
 * \brief Evolve the current state machine state when a byte is received.
 * \return This function returns a SM_Status execution code.
 * \param *pHandle is a pointer on a SM_Handle struct containing the current context of the state machine.
 * \param rcvdByte is the newly received byte that the state machine has to process.
 * 
 * This function evolve the current (reception) state machine state when a byte is received.
 * This function is designed to be called from from a USART interrupt routine for example.
 */
SM_Status SM_EvolveStateOnByteReception(SM_Handle *pHandle, uint8_t rcvdByte){
	SM_Status rv;
	SEM_Status mutexRv;
	SM_RcvState nextState;
	
	
	if((pHandle->rcvHandle.flagRcptOngoing) == 0){
		return SM_ERR;
	}
	
	/* We check if the state machine context is already accessed by another interrupt routine ...  */
	mutexRv = SEM_TryLock(&(pHandle->rcvHandle.contextAccessMutex));
	if((mutexRv != SEM_LOCKED) && (mutexRv != SEM_UNLOCKED)) return SM_ERR;
	
	if(mutexRv == SEM_UNLOCKED){
		/* If everything is okay we process normally the state ... */
		rv = SM_ComputeNextRcvState(pHandle, rcvdByte, &nextState);
		if(rv != SM_OK) return SM_ERR;
		
		rv = SM_MoveToNextRcvState(pHandle, nextState);
		if(rv != SM_OK) return SM_ERR;
		
		rv = SM_ApplyRcvState(pHandle, rcvdByte);
		if(rv != SM_OK) return SM_ERR;
		
		mutexRv = SEM_Release(&(pHandle->rcvHandle.contextAccessMutex));
		if(mutexRv != SEM_OK) return SM_ERR;
	}
	/* TODO !!!!!!! */
	else{
		/* If the reception context is already locked by another interrupt routine ...  */
		/* We enqueue a BUSY block in the data about to be sent to the computer ...     */
		//rv = SM_SendControlBlock(pHandle, SM_BUSY_BLOCK);
		//if(rv != SM_OK) return SM_ERR;
		return SM_BUSY;
	}
	
	return SM_OK;
}


/**
 * \fn SM_Status SM_IsDataAvail(SM_Handle *pHandle)
 * \brief This function is used to test if they are available received bytes in the current communication context.
 * \param *pHandle is a pointer on a SM_Handle struct containing the current context of the state machine.
 * \return This function returns a SM_Status execution code. If the returned value is #SM_OK, then data are ready and available. If #SM_NO, there is no available data bytes. Any other value indicates an error.
 * 
 * If you tested successfully that data bytes are available, you can then use the #SM_GetRcptBufferPtr() function to get the pointer on the available data.
 * This function is gonna return #SM_NO if the current block reception is not over.
 */
SM_Status SM_IsDataAvail(SM_Handle *pHandle){
	BUFF_Status buffRv;
	BUFF_Buffer *pBuffer;
	SM_Status rv;
	SM_RcvHandle *pRcvHandle;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	
	/* If there is no reception process ongoing, there is obviously no data avail ...  */
	if((pRcvHandle->flagRcptOngoing) == 0){
		return SM_NO;
	}
	
	/* If we are handling something different than a data block, there is no data avail ...  */
	if((pRcvHandle->currentBlockType) != SM_DATA_BLOCK){
		return SM_NO;
	}
	
	rv = SM_GetRcptBufferPtr(pHandle, &pBuffer);
	if(rv != SM_OK) return SM_ERR;
	
	buffRv = BUFF_IsEmpty(pBuffer);
	if((buffRv != BUFF_OK) && (buffRv != BUFF_NO)) return SM_ERR;
	
	if(BUFF_IsEmpty(pBuffer) == BUFF_NO){
		return SM_OK;
	}
	else{
		return SM_NO;
	}
}


SM_Status SM_IsAllDataRecieved(SM_Handle *pHandle){
	SM_RcvHandle *pRcvHandle;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	
	if((pRcvHandle->nbDataRcvd) == (pRcvHandle->nbDataExpected)){
		return SM_OK;
	}
	else{
		return SM_NO;
	}
}


/**
 * \fn SM_Status SM_GetRcptBufferPtr(SM_Handle *pHandle, BUFF_Buffer **ppBuffer)
 * \brief This function is used to get the pointer on the #BUFF_Buffer struct containing the received data bytes (in the case of a data block reception).
 * \param *pHandle is a pointer on a SM_Handle struct containing the current context of the state machine.
 * \param **ppBuffer is a pointer which points on a #BUFF_Buffer pointer. The pointed pointer is gonna be updated with the address of the buffer.
 * \return This function returns a SM_Status execution code.
 * 
 * Warning : Before using this function please make sure that the block has been received and that it is really a data block.
 * For example you can use the #SM_IsDataAvail() function for this purpose.
 */
SM_Status SM_GetRcptBufferPtr(SM_Handle *pHandle, BUFF_Buffer **ppBuffer){
	SM_RcvHandle *pRcvHandle;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	*ppBuffer = pRcvHandle->pBuffer;
	
	
	return SM_OK;
}


/**
 * \fn SM_Status SM_GetSendBufferPtr(SM_Handle *pHandle, BUFF_Buffer **ppBuffer)
 * \brief This function is used to get the pointer on the #BUFF_Buffer struct containing the bytes about to be sent by the transmission state machine.
 * \param *pHandle is a pointer on a SM_Handle struct containing the current context of the state machine.
 * \param **ppBuffer is a pointer which points on a #BUFF_Buffer pointer. The pointed pointer is gonna be updated with the address of the buffer.
 * \return This function returns a SM_Status execution code.
 */
SM_Status SM_GetSendBufferPtr(SM_Handle *pHandle, BUFF_Buffer **ppBuffer){
	SM_SendHandle *pSendHandle;
	
	
	pSendHandle = &(pHandle->sendHandle);
	*ppBuffer = pSendHandle->pBuffer;
	
	
	return SM_OK;
}


__attribute__((weak)) SM_Status SM_EnableRxneInterrupt_Callback(SM_Handle *pHandle){
	return SM_OK;
}


__attribute__((weak)) SM_Status SM_DisableRxneInterrupt_Callback(SM_Handle *pHandle){
	return SM_OK;
}


__attribute__((weak)) SM_Status SM_EnableTxeInterrupt_Callback(SM_Handle *pHandle){
	return SM_OK;
}


__attribute__((weak)) SM_Status SM_DisableTxeInterrupt_Callback(SM_Handle *pHandle){
	return SM_OK;
}


__attribute__((weak)) SM_Status SM_BlockRecievedCallback(SM_Handle *pHandle){
	return SM_OK;
}


__attribute__((weak)) SM_Status SM_CtrlBlockRecievedCallback(SM_Handle *pHandle){
	return SM_OK;
}


__attribute__((weak)) SM_Status SM_DataBlockRecievedCallback(SM_Handle *pHandle){
	return SM_OK;
}


__attribute__((weak)) SM_Status SM_COLD_RST_BLOCK_Callback(SM_Handle *pHandle){
	return SM_OK;
}


__attribute__((weak)) SM_Status SM_WARM_RST_BLOCK_Callback(SM_Handle *pHandle){
	return SM_OK;
}


__attribute__((weak)) SM_Status SM_UNKNOWN_BLOCK_Callback(SM_Handle *pHandle){
	return SM_OK;
}

__attribute__((weak)) SM_Status SM_ACK_BLOCK_ReceivedCallback(SM_Handle *pHandle){
	return SM_OK;
}


static SM_Status Apply_SM_ACK_BLOCK_Received(SM_Handle *pHandle){
	SM_Status rv;
	uint8_t dummy;
	
	
	pHandle->rcvHandle.flagAckRcptOccurred = 1;
	
	/* Check if a callback was currently expected ...  */
	/* If no ACK was expected, we are not going to change the state of the transmission state machine. */
	if((pHandle->rcvHandle.flagAckExpected) == 0){
		return SM_ERR;
	}
	
	/* We clear the ACK expected flag ...  */
	pHandle->rcvHandle.flagAckExpected = 0;
	
	/* We notify the transmission state machine that that ACK has been received ...  */
	pHandle->sendHandle.flagAckReceived = 1;
	
	/* Move the transmission state machine to the ACK received state ...  */
	rv = SM_EvolveStateOnByteTransmission(pHandle, &dummy);
	if(rv != SM_OK) return SM_ERR;
	
	
	return SM_OK;
}


/**
 * \fn static SM_Status SM_InitSend(SM_Handle *pHandle)
 * \brief Initializes the transmission state machine context.
 * \return This function returns a SM_Status execution code.
 * \param *pHandle is a pointer on a SM_Handle struct containing the current context of the transmission state machine.  (TODO : update doxy)
 */
static SM_Status SM_InitSend(SM_Handle *pHandle){
	SM_Status rv;
	SM_SendHandle *pSendHandle;
	SEM_Status semRv;
	
	
	pSendHandle = &(pHandle->sendHandle);
	
	rv = SM_DisableTxeInterrupt_Callback(pHandle);
	if(rv != SM_OK) return SM_ERR;	
	
	/* Initializing the mutexes ...  */
	semRv = SEM_Init(&(pSendHandle->sendProcessMutex), 1);
	if(semRv != SEM_OK) return SM_ERR;
	
	semRv = SEM_Init(&(pSendHandle->contextAccessMutex), 1);
	if(semRv != SEM_OK) return SM_ERR;
	
	/* We initialize the ACK expected flag ...  */
	pSendHandle->flagAckExpected = 0;
	pSendHandle->flagAckReceived = 0;
	pSendHandle->flagSendOngoing = 0;
	
	return SM_OK;
}


/**
 * \fn static SM_Status SM_EndBlockSendProcess(SM_Handle *pHandle)
 * \brief Perform all the necessary actions on the communication context when we went through all the steps to send a block.
 * \param *pHandle Is a pointer on a #SM_Handle struct containing the current communication context.
 * \return This function returns an #SM_Status error code to indicate if the function behaved as expected or not.
 * 
 * This function typically relases the mutex indicating that a send process in ongoing (#sendProcessMutex) and calls all the callback functions.
 * It also eventually starts a new transmission process if the reception state machine is waiting for an ACK to be sent.
 */
static SM_Status SM_EndBlockSendProcess(SM_Handle *pHandle){
	SM_Status rv;
	SEM_Status mutexRv;
	
	
	/* We release the mutex to enable a new send process to start ...  */
	mutexRv = SEM_Release(&(pHandle->sendHandle.sendProcessMutex));
	if(mutexRv != SEM_OK) return SM_ERR;
	
	pHandle->sendHandle.flagSendOngoing = 0;
	
	/* We call all the related callbacks ...  */
	if((pHandle->sendHandle.currentBlockType) == SM_ACK_BLOCK){
		//rv = SM_ACK_BLOCK_TransmittedCallback(pHandle);
		//if(rv != SM_OK) return SM_ERR;
	}
	
	rv = SM_BlockSentCallback(pHandle);
	if(rv != SM_OK) return SM_ERR;
	
	/* We start a new block transmission if the reception state machine is expecting an ACK to be sent ...  */
	if((pHandle->sendHandle.flagAckExpected) != 0){
		rv = SM_SendBlock(pHandle, NULL, SM_ACK_BLOCK);
		if(rv != SM_OK) return SM_ERR;
	}
	
		
	return SM_OK;
}


static SM_Status Apply_SM_ACK_BLOCK_Transmitted(SM_Handle *pHandle){
	SM_Status rv;
	
	
	/* Check if a callback was currently expected ...  */
	/* If no ACK transmission was expected, we are not going to change the state of the reception state machine. */
	if((pHandle->sendHandle.flagAckExpected) == 0){
		return SM_ERR;
	}
	
	/* We clear the ACK expected flag, we dont need anymore to send an ACK ...  */
	pHandle->sendHandle.flagAckExpected = 0;
	
	/* We notify the reception state machine that that ACK has been received ...  */
	pHandle->rcvHandle.flagAckTransmitted = 1;
	
	/* Move the reception state machine to the ACK received state ...  */
	rv = SM_EvolveStateOnByteReception(pHandle, 0x00);
	if(rv != SM_OK) return SM_ERR;
		
	
	return SM_OK;
}


/**
 * \fn SM_Status SM_SendBlock(SM_Handle *pHandle, BUFF_Buffer *pBuffer, SM_CtrlBlockType type)
 * \brief This non-blocking function starts a block transmission process to the computer.
 * \param *pHandle is a pointer on a #SM_Handle struct containing the current communication context.
 * \param *pBuffer is a pointer on a #BUFF_Buffer struct containing the potential payload of data to be sent to the computer. To save memory usage, this pointer is gonna be copied in the communication context, not the pointed data. Please make sure that the pointed content does not evolve until the transmission process is over. If there is no data to send (eg. control block) put NULL pointer instead.
 * \param type Is the type of block to be sent. It has to be a member of the #SM_CtrlBlockType enum.
 * \return This function returns an #SM_Status error code. If the returned value is #SM_OK then the block transmission process has been started. If #SM_BUSY it means that the transmission process has not been started because the state machine is already busy with another block transmission, please try again later. any other value indicates an error during the process.
 * 
 * This function gather the data to be sent and initiates the transmission state machine.
 * While the data is not completely sent and ACKed by the computer, it is not possible to initiate a new transaction. If this happen, this function will exit and return #SM_BUSY code.
 */
SM_Status SM_SendBlock(SM_Handle *pHandle, BUFF_Buffer *pBuffer, SM_CtrlBlockType type){
	SEM_Status mutexRv;
	SM_Status rv;
	SM_SendHandle *pSendHandle;
	uint8_t dummy;
	
	
	pSendHandle = &(pHandle->sendHandle);
	
	mutexRv = SEM_TryLock(&(pSendHandle->sendProcessMutex));
	if((mutexRv != SEM_LOCKED) && (mutexRv != SEM_UNLOCKED)) return SM_ERR;
	
	
	if(mutexRv == SEM_UNLOCKED){
		pSendHandle->flagEmpty = 0;
		pSendHandle->flagSendOngoing = 1;
		pSendHandle->pBuffer = pBuffer;
		pSendHandle->currentBlockType = type;
		pSendHandle->currentState = SM_SENDSTATE_INIT;
		
		rv = SM_ApplySendState(pHandle, &dummy);
		if(rv != SM_OK) return SM_ERR;
		
		rv = SM_EnableTxeInterrupt_Callback(pHandle);
		if(rv != SM_OK) return SM_ERR;
	}
	else{
		/* TODO */
		
		return SM_BUSY;
	}
	
	
	return SM_OK;
}


/**
 * \fn __attribute__((weak)) SM_Status SM_BlockSentCallback(SM_Handle *pHandle)
 * \brief Callback function when a block has been sent.
 * \param *pHandle Is a pointer on a #SM_Handle struct containing the current communication context.
 * \return This function has to return an #SM_Status error code to indicate if the function behaved as expected or not.
 * 
 * Warning : A new call to #SM_SendBlock() is actually not possible inside this functions because the mutex is going to be locked. (TODO: Is it still true ??)
 */
__attribute__((weak)) SM_Status SM_BlockSentCallback(SM_Handle *pHandle){
	return SM_OK;
}


/**
 * \fn __attribute__((weak)) SM_Status SM_ACK_BLOCK_TransmittedCallback(SM_Handle *pHandle)
 * \brief Callback function when the block transmission process is over and anc ACK transmission has occured during the process.
 * \param *pHandle Is a pointer on a #SM_Handle struct containing the current communication context.
 * \return This function has to return an #SM_Status error code to indicate if the function behaved as expected or not.
 */
__attribute__((weak)) SM_Status SM_ACK_BLOCK_TransmittedCallback(SM_Handle *pHandle){
	return SM_OK;
}


/**
 * \fn SM_Status SM_EvolveStateOnByteTransmission(SM_Handle *pHandle, uint8_t *pByteToSend)
 * \param *pHandle Is a pointer on a #SM_Handle struct containing the current communication context.
 * \param *pByteToSend is a pointer on the place where to put the next byte to be sent.
 * \return This function returns an #SM_Status error code to indicate if the function behaved as expected or not. Especially it returns #SM_EMPTY code it there is nothing more to be sent.
 * 
 * This public function is designed to be called when the system is ready to transmit a new byte in the direction of the computer.
 * The function evolves the transmission state machine state and returns through a pointer (#*pByteToSend) the next byte which has to be sent.
 * This function can typically be called in the TX Empty interrupt routine.
 */
SM_Status SM_EvolveStateOnByteTransmission(SM_Handle *pHandle, uint8_t *pByteToSend){
	SM_Status rv;
	SEM_Status mutexRv;
	SM_SendState nextState;
	
	
	if((pHandle->sendHandle.flagSendOngoing) == 0){
		return SM_ERR;
	}	
	
	mutexRv = SEM_TryLock(&(pHandle->sendHandle.contextAccessMutex));
	if((mutexRv != SEM_LOCKED) && (mutexRv != SEM_UNLOCKED)) return SM_ERR;
	
	if(mutexRv == SEM_UNLOCKED){
		rv = SM_ComputeNextSendState(pHandle, &nextState);
		if(rv != SM_OK) return SM_ERR;
		
		rv = SM_MoveToNextSendState(pHandle, nextState);
		if(rv != SM_OK) return SM_ERR;
		
		rv = SM_ApplySendState(pHandle, pByteToSend);
		if(rv != SM_OK) return SM_ERR;
		
		if((pHandle->sendHandle.flagEmpty) != 0){
			rv = SM_DisableTxeInterrupt_Callback(pHandle);    /*  It means that we have just sent the last byte of this transmission process. There is nothing more to be sent. */
			if(rv != SM_OK) return SM_ERR;
		}
		
		mutexRv = SEM_Release(&(pHandle->sendHandle.contextAccessMutex));
		if(mutexRv != SEM_OK) return SM_ERR;
	}
	else{
		/* TODO : Sending busy block to computer ?? ...  */
		/* TODO : Udpating Doxygen header ...  */
		return SM_BUSY;
	}
	
	
	return SM_OK;
}


/* Private functions definitions ... */

static SM_Status SM_MoveToNextRcvState(SM_Handle *pHandle, SM_RcvState nextState){
	SM_RcvHandle *pRcvHandle;
	
	
	pRcvHandle = &(pHandle->rcvHandle);	
	pRcvHandle->currentState = nextState;
	
	return SM_OK;
}


static SM_Status SM_ComputeNextRcvState(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState){
	SM_RcvHandle *pRcvHandle;
	SM_Status rv;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	
	switch(pRcvHandle->currentState){
		case SM_RCVSTATE_INIT:
			rv = SM_ComputeNextStateFrom_SM_RCVSTATE_INIT(pHandle, rcvdByte, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_RCVSTATE_CTRL_BYTE:
			rv = SM_ComputeNextStateFrom_SM_RCVSTATE_CTRL_BYTE(pHandle, rcvdByte, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_RCVSTATE_LEN_BYTE1:
			rv = SM_ComputeNextStateFrom_SM_RCVSTATE_LEN_BYTE1(pHandle, rcvdByte, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_RCVSTATE_LEN_BYTE2:
			rv = SM_ComputeNextStateFrom_SM_RCVSTATE_LEN_BYTE2(pHandle, rcvdByte, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_RCVSTATE_LEN_BYTE3:
			rv = SM_ComputeNextStateFrom_SM_RCVSTATE_LEN_BYTE3(pHandle, rcvdByte, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_RCVSTATE_DATA:
			rv = SM_ComputeNextStateFrom_SM_RCVSTATE_DATA(pHandle, rcvdByte, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_RCVSTATE_CHECK:
			rv = SM_ComputeNextStateFrom_SM_RCVSTATE_CHECK(pHandle, rcvdByte, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_RCVSTATE_TRANSMITTED_ACK:
			rv = SM_ComputeNextStateFrom_SM_RCVSTATE_TRANSMITTED_ACK(pHandle, rcvdByte, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_RCVSTATE_ACK_CTRL_BYTE:
			rv = SM_ComputeNextStateFrom_SM_RCVSTATE_ACK_CTRL_BYTE(pHandle, rcvdByte, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_RCVSTATE_ACK_CHECK:
			rv = SM_ComputeNextStateFrom_SM_RCVSTATE_ACK_CHECK(pHandle, rcvdByte, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		default:
			return SM_ERR;
	}
	
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_INIT(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState){
	*pNextState = SM_RCVSTATE_CTRL_BYTE;
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_TRANSMITTED_ACK(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState){
	*pNextState = SM_RCVSTATE_TRANSMITTED_ACK;
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_CTRL_BYTE(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState){
	SM_CtrlBlockType type;
	SM_RcvHandle *pRcvHandle;
		
	
	pRcvHandle = &(pHandle->rcvHandle);
	type = pRcvHandle->currentBlockType;
	
	switch(type){
		case SM_DATA_BLOCK:
			*pNextState = SM_RCVSTATE_LEN_BYTE1;
			break;
		
		case SM_COLD_RST_BLOCK:
			*pNextState = SM_RCVSTATE_CHECK;
			break;
			
		case SM_ACK_BLOCK:
			*pNextState = SM_RCVSTATE_CHECK;
			break;
		
		case SM_UNKNOWN_BLOCK:
			return SM_ERR;
			break;
		
		default:
			return SM_ERR;
	}
	
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_LEN_BYTE1(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState){
	*pNextState = SM_RCVSTATE_LEN_BYTE2;
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_LEN_BYTE2(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState){
	*pNextState = SM_RCVSTATE_LEN_BYTE3;
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_LEN_BYTE3(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState){
	if((pHandle->rcvHandle.nbDataExpected) == 0){
		*pNextState = SM_RCVSTATE_CHECK;
	}
	else{
		*pNextState = SM_RCVSTATE_DATA;
	}
	
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_DATA(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState){
	SM_RcvHandle *pRcvHandle;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	
	if((pRcvHandle->nbDataRcvd) < (pRcvHandle->nbDataExpected)){
		*pNextState = SM_RCVSTATE_DATA;
	}
	else if((pRcvHandle->nbDataRcvd) == (pRcvHandle->nbDataExpected)){
		*pNextState = SM_RCVSTATE_CHECK;
	}
	else{
		return SM_ERR;
	}
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_CHECK(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState){
	SM_Status rv;
	
	
	/* We see if the block we have just received needs an ACK response ... */
	rv = SM_DoesThisBlockNeedAnAck(pHandle->rcvHandle.currentBlockType);
	if((rv != SM_OK) && (rv != SM_NO)) return SM_ERR;
	
	
	if((pHandle->rcvHandle.flagAckExpected) != 0){   /* If we have to receive an ACK, we go straight to the receive ACK branch ...  */
		*pNextState = SM_RCVSTATE_ACK_CTRL_BYTE;
	}
	else{
		if(rv == SM_OK){
			if((pHandle->rcvHandle.flagAckTransmitted) != 0){    
				*pNextState = SM_RCVSTATE_TRANSMITTED_ACK;
			}
			else{
				*pNextState = SM_RCVSTATE_CHECK;
			}
		}
		else{
			*pNextState = SM_RCVSTATE_CHECK;        /* We stay ...   */
		}
	}
	
	
	return SM_OK;	
}


static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_ACK_CTRL_BYTE(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState){
	*pNextState = SM_RCVSTATE_ACK_CHECK;
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_RCVSTATE_ACK_CHECK(SM_Handle *pHandle, uint8_t rcvdByte, SM_RcvState *pNextState){
	SM_Status rv;
	
	
	/* We see if we need to transmit an ACK ...  */
	rv = SM_DoesThisBlockNeedAnAck(pHandle->rcvHandle.currentBlockType);
	if((rv != SM_OK) && (rv != SM_NO)) return SM_ERR;
	
	if(rv == SM_OK){                             /* If we expect to send an ACK ...                   */
		if((pHandle->rcvHandle.flagAckTransmitted) != 0){ 
			*pNextState = SM_RCVSTATE_TRANSMITTED_ACK;      /* If we already transmitted it, we go the the correspoding state ...            */
		}
		else{
			*pNextState = SM_RCVSTATE_ACK_CHECK;     /* If we didn't, we stay here to wait for it ...  */
		}
	}
	else{
		*pNextState = SM_RCVSTATE_ACK_CHECK;        /* If we are not expecting any ACK, we stay here, its the end ...                                   */
	}
	
	
	return SM_OK;
}


static SM_Status SM_ApplyRcvState(SM_Handle *pHandle, uint8_t rcvdByte){
	SM_RcvHandle *pRcvHandle;
	SM_Status rv;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	
	switch(pRcvHandle->currentState){
		case SM_RCVSTATE_INIT:
			rv = SM_ApplyState_SM_RCVSTATE_INIT(pHandle);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_RCVSTATE_CTRL_BYTE:
			rv = SM_ApplyState_SM_RCVSTATE_CTRL_BYTE(pHandle, rcvdByte);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_RCVSTATE_LEN_BYTE1:
			rv = SM_ApplyState_SM_RCVSTATE_LEN_BYTE1(pHandle, rcvdByte);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_RCVSTATE_LEN_BYTE2:
			rv = SM_ApplyState_SM_RCVSTATE_LEN_BYTE2(pHandle, rcvdByte);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_RCVSTATE_LEN_BYTE3:
			rv = SM_ApplyState_SM_RCVSTATE_LEN_BYTE3(pHandle, rcvdByte);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_RCVSTATE_DATA:
			rv = SM_ApplyState_SM_RCVSTATE_DATA(pHandle, rcvdByte);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_RCVSTATE_CHECK:
			rv = SM_ApplyState_SM_RCVSTATE_CHECK(pHandle, rcvdByte);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_RCVSTATE_TRANSMITTED_ACK:
			rv = SM_ApplyState_SM_RCVSTATE_TRANSMITTED_ACK(pHandle, rcvdByte);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_RCVSTATE_ACK_CTRL_BYTE:
			rv = SM_ApplyState_SM_RCVSTATE_ACK_CTRL_BYTE(pHandle, rcvdByte);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_RCVSTATE_ACK_CHECK:
			rv = SM_ApplyState_SM_RCVSTATE_ACK_CHECK(pHandle, rcvdByte);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		default:
			return SM_ERR;
	}
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_RCVSTATE_INIT(SM_Handle *pHandle){
	SM_RcvHandle *pRcvHandle;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	
	pRcvHandle->nbDataRcvd = 0;
	pRcvHandle->nbDataExpected = 0;
	pRcvHandle->currentBlockType = SM_UNKNOWN_BLOCK;
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_RCVSTATE_TRANSMITTED_ACK(SM_Handle *pHandle, uint8_t rcvdByte){
	SM_Status rv;
	
	
	/* Releasing ACK Transmitted flag ...  */
	pHandle->rcvHandle.flagAckTransmitted = 0;
	
	/* Ending the block receiving process ...  */
	rv = SM_EndBlockRcvProcess(pHandle);
	if(rv != SM_OK) return SM_ERR;
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_RCVSTATE_CTRL_BYTE(SM_Handle *pHandle, uint8_t rcvdByte){
	SM_RcvHandle *pRcvHandle;
	BUFF_Buffer *pBuffer;
	BUFF_Status buffRv;
	SM_Status rv;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	pRcvHandle->currentBlockType = rcvdByte;
	
	
	/* If we are about to receive a data block, we prepare the buffer ...  */
	if(rcvdByte == SM_DATA_BLOCK){
		/* We get a pointer on the reception buffer ...  */
		rv = SM_GetRcptBufferPtr(pHandle, &pBuffer);
		if(rv != SM_OK) return SM_ERR;
		
		
		if((pRcvHandle->pBuffer) != NULL){
			buffRv = BUFF_Init(pBuffer);
			if(buffRv != BUFF_OK) return SM_ERR;
		}
	}
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_RCVSTATE_LEN_BYTE1(SM_Handle *pHandle, uint8_t rcvdByte){
	SM_RcvHandle *pRcvHandle;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	pRcvHandle->nbDataExpected += ((uint32_t)(rcvdByte) << 16) & (uint32_t)(0x00FF0000);
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_RCVSTATE_LEN_BYTE2(SM_Handle *pHandle, uint8_t rcvdByte){
	SM_RcvHandle *pRcvHandle;
	
	pRcvHandle = &(pHandle->rcvHandle);
	pRcvHandle->nbDataExpected += ((uint32_t)(rcvdByte) << 8) & (uint32_t)(0x0000FF00);
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_RCVSTATE_LEN_BYTE3(SM_Handle *pHandle, uint8_t rcvdByte){
	SM_RcvHandle *pRcvHandle;
	
	pRcvHandle = &(pHandle->rcvHandle);
	pRcvHandle->nbDataExpected += ((uint32_t)(rcvdByte)) & (uint32_t)(0x000000FF);
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_RCVSTATE_DATA(SM_Handle *pHandle, uint8_t rcvdByte){
	BUFF_Status buffRv;
	BUFF_Buffer *pBuffer;
	SM_Status rv;
	SM_RcvHandle *pRcvHandle;
	
	
	pRcvHandle = &(pHandle->rcvHandle);
	
	
	/* We get a pointer on the reception buffer ...  */
	rv = SM_GetRcptBufferPtr(pHandle, &pBuffer);
	if(rv != SM_OK) return SM_ERR;
	
	buffRv = BUFF_Enqueue(pBuffer, rcvdByte);
	if(buffRv != BUFF_OK) return SM_ERR;
	
	pRcvHandle->nbDataRcvd ++;
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_RCVSTATE_CHECK(SM_Handle *pHandle, uint8_t rcvdByte){
	SM_Status rv, rv2;
	
	
	/* TODO checking integrity ... */

	/* Checking if we are in the last step of the transsmission process (are we expecting to receive an ACK ?) ...  */
	rv = SM_DoesThisBlockNeedAnAck(pHandle->rcvHandle.currentBlockType);
	if((rv != SM_OK) && (rv != SM_NO)) return SM_ERR;	
	
	/* If we have just received an ACK and if it was expected by the transmission state machine ...  */
	if(((pHandle->rcvHandle.currentBlockType) == SM_ACK_BLOCK) && ((pHandle->rcvHandle.flagAckExpected) != 0)){
		rv2 = Apply_SM_ACK_BLOCK_Received(pHandle);
		if(rv2 != SM_OK) return SM_ERR;
	}
	
	/* Do we have to receive an ACK ?? ...  */
	if((pHandle->rcvHandle.flagAckExpected) != 0){
		//pHandle->sendHandle.flagAckExpected = 0;
	}
	else{
		if(rv == SM_NO){
			rv = SM_EndBlockRcvProcess(pHandle);
			if(rv != SM_OK) return SM_ERR;
		}
		else{
			/* Setting wait flags ...  */
			pHandle->sendHandle.flagAckExpected = 1;
			
			/* We try to start a new block reception in order to receive the ACK block ...                                                                                    */
			/* It is normal here is SM_SendBlock() returns SM_BUSY (we are in full duplex mode, a transmission might be already happening).                                   */
			/* Thats why we use flagAckExpected, thats way the the transmission state machine will know it has to start a new transmission (with ACK) when it gonna be ready. */
			rv = SM_SendBlock(pHandle, NULL, SM_ACK_BLOCK);
			if((rv != SM_OK) && (rv != SM_BUSY)) return SM_ERR;
		}
	}
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_RCVSTATE_ACK_CTRL_BYTE(SM_Handle *pHandle, uint8_t rcvdByte){
	/* TODO : improve the verification of the ACK byte */
	
	
	if(rcvdByte != SM_ACK_BLOCK){
		return SM_ERR;
	}
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_RCVSTATE_ACK_CHECK(SM_Handle *pHandle, uint8_t rcvdByte){
	SM_Status rv;
	
	
	/* TODO : LRC verification of the received ACK ...  */
	
	rv = Apply_SM_ACK_BLOCK_Received(pHandle);
	if(rv != SM_OK) return SM_ERR;
	
	
	rv = SM_DoesThisBlockNeedAnAck(pHandle->rcvHandle.currentBlockType);
	if((rv != SM_OK) && (rv != SM_NO)) return SM_ERR;
	
	/* Do we need to transmit an ACK ?? ...  */
	if(rv == SM_OK){
		pHandle->sendHandle.flagAckExpected = 1;
		
		rv = SM_SendBlock(pHandle, NULL, SM_ACK_BLOCK);
		if((rv != SM_OK) && (rv != SM_BUSY)) return SM_ERR;
	}
	else{
		rv = SM_EndBlockRcvProcess(pHandle);
		if(rv != SM_OK) return SM_ERR;
	}
	
	return SM_OK;
}


static SM_Status SM_ApplySendState(SM_Handle *pHandle, uint8_t *pByteToSend){
	SM_SendHandle *pSendHandle;
	SM_Status rv;
	
	
	pSendHandle = &(pHandle->sendHandle);
	
	switch(pSendHandle->currentState){
		case SM_SENDSTATE_INIT:
			rv = SM_ApplyState_SM_SENDSTATE_INIT(pHandle);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_SENDSTATE_CTRL_BYTE:
			rv = SM_ApplyState_SM_SENDSTATE_CTRL_BYTE(pHandle, pByteToSend);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_SENDSTATE_LEN_BYTE1:
			rv = SM_ApplyState_SM_SENDSTATE_LEN_BYTE1(pHandle, pByteToSend);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_SENDSTATE_LEN_BYTE2:
			rv = SM_ApplyState_SM_SENDSTATE_LEN_BYTE2(pHandle, pByteToSend);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_SENDSTATE_LEN_BYTE3:
			rv = SM_ApplyState_SM_SENDSTATE_LEN_BYTE3(pHandle, pByteToSend);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_SENDSTATE_DATA:
			rv = SM_ApplyState_SM_SENDSTATE_DATA(pHandle, pByteToSend);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_SENDSTATE_CHECK:
			rv = SM_ApplyState_SM_SENDSTATE_CHECK(pHandle, pByteToSend);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_SENDSTATE_RCVD_ACK:
			rv = SM_ApplyState_SM_SENDSTATE_RCVD_ACK(pHandle, pByteToSend);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_SENDSTATE_ACK_CTRL_BYTE:
			rv = SM_ApplyState_SM_SENDSTATE_ACK_CTRL_BYTE(pHandle, pByteToSend);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_SENDSTATE_ACK_CHECK:
			rv = SM_ApplyState_SM_SENDSTATE_ACK_CHECK(pHandle, pByteToSend);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		default:
			return SM_ERR;
	}
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_SENDSTATE_INIT(SM_Handle *pHandle){
	/* Initializing flags ...  */
	pHandle->sendHandle.flagAckReceived = 0;
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_SENDSTATE_RCVD_ACK(SM_Handle *pHandle, uint8_t *pByteToSend){
	SM_Status rv;
	
	
	/* Releasing ACK Received flag ...  */
	pHandle->sendHandle.flagAckReceived = 0;
	
	/* Ending the prock sending process ...  */
	rv = SM_EndBlockSendProcess(pHandle);
	if(rv != SM_OK) return SM_ERR;
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_SENDSTATE_CTRL_BYTE(SM_Handle *pHandle, uint8_t *pByteToSend){
	SM_SendHandle *pSendHandle;
	
	
	pSendHandle = &(pHandle->sendHandle);
	*pByteToSend = pSendHandle->currentBlockType;
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_SENDSTATE_LEN_BYTE1(SM_Handle *pHandle, uint8_t *pByteToSend){
	BUFF_Status buffRv;
	BUFF_Buffer *pBuffer;
	SM_Status rv;
	uint32_t dataSize;
	
	
	rv = SM_GetSendBufferPtr(pHandle, &pBuffer);
	if(rv != SM_OK) return SM_ERR;
	
	buffRv = BUFF_GetCurrentSize(pBuffer, &dataSize);
	if(buffRv != BUFF_OK) return SM_ERR;
	
	*pByteToSend = (uint8_t)((dataSize >> 16) & (uint32_t)(0x000000FF));
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_SENDSTATE_LEN_BYTE2(SM_Handle *pHandle, uint8_t *pByteToSend){
	BUFF_Status buffRv;
	BUFF_Buffer *pBuffer;
	SM_Status rv;
	uint32_t dataSize;
	
	
	rv = SM_GetSendBufferPtr(pHandle, &pBuffer);
	if(rv != SM_OK) return SM_ERR;
	
	buffRv = BUFF_GetCurrentSize(pBuffer, &dataSize);
	if(buffRv != BUFF_OK) return SM_ERR;
	
	*pByteToSend = (uint8_t)((dataSize >> 8) & (uint32_t)(0x000000FF));
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_SENDSTATE_LEN_BYTE3(SM_Handle *pHandle, uint8_t *pByteToSend){
	BUFF_Status buffRv;
	BUFF_Buffer *pBuffer;
	SM_Status rv;
	uint32_t dataSize;
	
	
	rv = SM_GetSendBufferPtr(pHandle, &pBuffer);
	if(rv != SM_OK) return SM_ERR;
		
	buffRv = BUFF_GetCurrentSize(pBuffer, &dataSize);
	if(buffRv != BUFF_OK) return SM_ERR;
	
	*pByteToSend = (uint8_t)((dataSize >> 0) & (uint32_t)(0x000000FF));
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_SENDSTATE_DATA(SM_Handle *pHandle, uint8_t *pByteToSend){
	BUFF_Status buffRv;
	BUFF_Buffer *pBuffer;
	SM_Status rv;
	
	
	rv = SM_GetSendBufferPtr(pHandle, &pBuffer);
	if(rv != SM_OK) return SM_ERR;
	
	buffRv = BUFF_Dequeue(pBuffer, pByteToSend);
	if(buffRv != BUFF_OK) return SM_ERR;
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_SENDSTATE_CHECK(SM_Handle *pHandle, uint8_t *pByteToSend){
	SM_Status rv;
	
	
	/* TODO : Computing LRC check ... */
	
	*pByteToSend = 0x00;
	
	/* If we have just sent an ACK and if it was expected by the reception state machine ...  */
	if(((pHandle->sendHandle.currentBlockType) == SM_ACK_BLOCK) && ((pHandle->sendHandle.flagAckExpected) != 0)){
		rv = Apply_SM_ACK_BLOCK_Transmitted(pHandle);
		if(rv != SM_OK) return SM_ERR;
	}
	
	/* Checking if we are in the last step of the transmission process (are we expecting to receive an ACK ?) ...  */
	rv = SM_DoesThisBlockNeedAnAck(pHandle->sendHandle.currentBlockType);
	if((rv != SM_OK) && (rv != SM_NO)) return SM_ERR;
	
	
	/* Do we have to send an ACK ?? ...  */
	if((pHandle->sendHandle.flagAckExpected) != 0){
		
	}
	else{
		/* We set the flag indicating that we have no more data to be sent ...  */
		pHandle->sendHandle.flagEmpty = 1;
		
		if(rv == SM_NO){
			/* We do not have to send an ACK and we do not need to receive an ACK  ...  */
			rv = SM_EndBlockSendProcess(pHandle);
			if(rv != SM_OK) return SM_ERR;
		}
		else{
			/* We do not have to send an ACK and we need to receive an ACK  ...  */
			pHandle->rcvHandle.flagAckExpected = 1;
			
			//rv = SM_ReceiveBlockWithSameBuffer(pHandle);             /* We start a new block reception process if it is not already ongoing ...  */
			rv = SM_ReceiveBlock(pHandle, NULL);  /* We do not need a buffer for the reception, we expect to receive an ACK. */
			if((rv != SM_OK) && (rv != SM_BUSY)) return SM_ERR;
		}
	}
	
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_SENDSTATE_ACK_CTRL_BYTE(SM_Handle *pHandle, uint8_t *pByteToSend){
	*pByteToSend = SM_ACK_BLOCK;
	
	return SM_OK;
}


static SM_Status SM_ApplyState_SM_SENDSTATE_ACK_CHECK(SM_Handle *pHandle, uint8_t *pByteToSend){
	SM_Status rv;
	/* TODO implement LRC */
	
	*pByteToSend = 0x00;
	
	/* We set the flag indicating that we have no more data to be sent ...  */
	pHandle->sendHandle.flagEmpty = 1;
	
	rv = Apply_SM_ACK_BLOCK_Transmitted(pHandle);
	if(rv != SM_OK) return SM_ERR;
	
	
	rv = SM_DoesThisBlockNeedAnAck(pHandle->sendHandle.currentBlockType);
	if((rv != SM_OK) && (rv != SM_NO)) return SM_ERR;
	
	/* Do we need to receive an ACK ?? ...  */
	if(rv == SM_OK){
		pHandle->rcvHandle.flagAckExpected = 1;
		
		rv = SM_ReceiveBlock(pHandle, NULL);
		if((rv != SM_OK) && (rv != SM_BUSY)) return SM_ERR;   /* TODO : Behavior to have if it is busy ? */
	}
	else{
		rv = SM_EndBlockSendProcess(pHandle);
		if(rv != SM_OK) return SM_ERR;
	}
	
	return SM_OK;
}


static SM_Status SM_ComputeNextSendState(SM_Handle *pHandle, SM_SendState *pNextState){
	SM_SendHandle *pSendHandle;
	SM_Status rv;
	
	
	pSendHandle = &(pHandle->sendHandle);
	
	switch(pSendHandle->currentState){
		case SM_SENDSTATE_INIT:
			rv = SM_ComputeNextStateFrom_SM_SENDSTATE_INIT(pHandle, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_SENDSTATE_CTRL_BYTE:
			rv = SM_ComputeNextStateFrom_SM_SENDSTATE_CTRL_BYTE(pHandle, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_SENDSTATE_LEN_BYTE1:
			rv = SM_ComputeNextStateFrom_SM_SENDSTATE_LEN_BYTE1(pHandle, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_SENDSTATE_LEN_BYTE2:
			rv = SM_ComputeNextStateFrom_SM_SENDSTATE_LEN_BYTE2(pHandle, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_SENDSTATE_LEN_BYTE3:
			rv = SM_ComputeNextStateFrom_SM_SENDSTATE_LEN_BYTE3(pHandle, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_SENDSTATE_DATA:
			rv = SM_ComputeNextStateFrom_SM_SENDSTATE_DATA(pHandle, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		case SM_SENDSTATE_CHECK:
			rv = SM_ComputeNextStateFrom_SM_SENDSTATE_CHECK(pHandle, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_SENDSTATE_ACK_CTRL_BYTE:
			rv = SM_ComputeNextStateFrom_SM_SENDSTATE_ACK_CTRL_BYTE(pHandle, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_SENDSTATE_ACK_CHECK:
			rv = SM_ComputeNextStateFrom_SM_SENDSTATE_ACK_CHECK(pHandle, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
			
		case SM_SENDSTATE_RCVD_ACK:
			rv = SM_ComputeNextStateFrom_SM_SENDSTATE_RCVD_ACK(pHandle, pNextState);
			if(rv != SM_OK) return SM_ERR;
			break;
		
		default:
			return SM_ERR;
	}
	
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_INIT(SM_Handle *pHandle, SM_SendState *pNextState){
	*pNextState = SM_SENDSTATE_CTRL_BYTE;
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_RCVD_ACK(SM_Handle *pHandle, SM_SendState *pNextState){
	*pNextState = SM_SENDSTATE_RCVD_ACK;
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_CTRL_BYTE(SM_Handle *pHandle, SM_SendState *pNextState){
	SM_CtrlBlockType type;
	SM_SendHandle *pSendHandle;
		
	
	pSendHandle = &(pHandle->sendHandle);
	type = pSendHandle->currentBlockType;
	
	switch(type){
		case SM_DATA_BLOCK:
			*pNextState = SM_SENDSTATE_LEN_BYTE1;
			break;
		
		case SM_COLD_RST_BLOCK:
			*pNextState = SM_SENDSTATE_CHECK;
			break;
			
		case SM_BUSY_BLOCK:
			*pNextState = SM_SENDSTATE_CHECK;
			break;
		
		case SM_ACK_BLOCK:
			*pNextState = SM_SENDSTATE_CHECK;
			break;
		
		case SM_UNKNOWN_BLOCK:
			return SM_ERR;
			break;
		
		default:
			return SM_ERR;
	}
	
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_LEN_BYTE1(SM_Handle *pHandle, SM_SendState *pNextState){
	*pNextState = SM_SENDSTATE_LEN_BYTE2;
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_LEN_BYTE2(SM_Handle *pHandle, SM_SendState *pNextState){
	*pNextState = SM_SENDSTATE_LEN_BYTE3;
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_LEN_BYTE3(SM_Handle *pHandle, SM_SendState *pNextState){
	BUFF_Status buffRv;
	BUFF_Buffer *pBuffer;
	SM_Status rv;
	
	
	rv = SM_GetSendBufferPtr(pHandle, &pBuffer);
	if(rv != SM_OK) return SM_ERR;
	
	buffRv = BUFF_IsEmpty(pBuffer);
	if((buffRv != BUFF_OK) && (buffRv != BUFF_NO)) return SM_ERR;
	
	if(buffRv == BUFF_OK){
		*pNextState = SM_SENDSTATE_CHECK;
	}
	else{
		*pNextState = SM_SENDSTATE_DATA;
	}
	
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_DATA(SM_Handle *pHandle, SM_SendState *pNextState){
	BUFF_Status buffRv;
	BUFF_Buffer *pBuffer;
	SM_Status rv;
	
	
	rv = SM_GetSendBufferPtr(pHandle, &pBuffer);
	if(rv != SM_OK) return SM_ERR;
	
	buffRv = BUFF_IsEmpty(pBuffer);
	if((buffRv != BUFF_OK) && (buffRv != BUFF_NO)) return SM_ERR;
	
	/* If the buffer containing the payload data to be sent is not empty */
	if(buffRv == BUFF_NO){
		*pNextState = SM_SENDSTATE_DATA;
	}
	else{
		*pNextState = SM_SENDSTATE_CHECK;
	}
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_CHECK(SM_Handle *pHandle, SM_SendState *pNextState){
	SM_Status rv;
	
	
	/* We see if the block we have just sent needs an ACK... */
	rv = SM_DoesThisBlockNeedAnAck(pHandle->sendHandle.currentBlockType);
	if((rv != SM_OK) && (rv != SM_NO)) return SM_ERR;
	
	if((pHandle->sendHandle.flagAckExpected) != 0){   /* If we have to send an ACK, we go straight to the send ACK branch ...  */
		*pNextState = SM_SENDSTATE_ACK_CTRL_BYTE;
	}
	else{
		if(rv == SM_OK){                             /* Else, if we received an ACK and it was expected ...                   */
			if((pHandle->sendHandle.flagAckReceived) != 0){ 
				*pNextState = SM_SENDSTATE_RCVD_ACK;
			}
			else{
				*pNextState = SM_SENDSTATE_CHECK;    /* Else, if we do not received an ACK and it was expected ...            */
			}
		}
		else{
			*pNextState = SM_SENDSTATE_CHECK;        /* If we are not expecting any ACK ...                                   */
		}
	}
	
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_ACK_CTRL_BYTE(SM_Handle *pHandle, SM_SendState *pNextState){
	*pNextState = SM_SENDSTATE_ACK_CHECK;
	
	return SM_OK;
}


static SM_Status SM_ComputeNextStateFrom_SM_SENDSTATE_ACK_CHECK(SM_Handle *pHandle, SM_SendState *pNextState){
	SM_Status rv;
	
	
	/* We see if we need to receive an ACK ...  */
	rv = SM_DoesThisBlockNeedAnAck(pHandle->sendHandle.currentBlockType);
	if((rv != SM_OK) && (rv != SM_NO)) return SM_ERR;
	
	if(rv == SM_OK){                             /* If we expect to receive an ACK ...                   */
		if((pHandle->sendHandle.flagAckReceived) != 0){ 
			*pNextState = SM_SENDSTATE_RCVD_ACK;      /* If we received it, we go the the correspoding state ...            */
		}
		else{
			*pNextState = SM_SENDSTATE_ACK_CHECK;     /* If we didn't, we stay here to wait for it ...  */
		}
	}
	else{
		*pNextState = SM_SENDSTATE_ACK_CHECK;        /* If we are not expecting any ACK, we stay here, its the end ...                                   */
	}
	
	
	return SM_OK;
}


static SM_Status SM_MoveToNextSendState(SM_Handle *pHandle, SM_SendState nextState){
	SM_SendHandle *pSendHandle;
	
	
	pSendHandle = &(pHandle->sendHandle);	
	pSendHandle->currentState = nextState;
	
	return SM_OK;
}


/**
 * \fn static SM_Status SM_DoesThisBlockNeedAnAck(SM_CtrlBlockType type)
 * \brief Does the type of block described by teh #type parameter needs to be ackitted by the computer ?.
 * \param type is of type SM_CtrlBlockType. It encodes the type of the control byte of the block we want to know if it has to be ACKed.
 * \return This function returns #SM_OK if the type of block described by #type needs to be ACKed. It returns #SM_NO if there is no need for an ACK. Any other value indicates an error.
 */
static SM_Status SM_DoesThisBlockNeedAnAck(SM_CtrlBlockType type){
	switch(type){
		case SM_DATA_BLOCK:
			return SM_OK;
			break;
		
		case SM_COLD_RST_BLOCK:
			return SM_OK;
			break;
			
		case SM_BUSY_BLOCK:
			return SM_NO;
			break;
			
		case SM_ACK_BLOCK:
			return SM_NO;
			break;
		
		default:
			return SM_ERR;
	}
}

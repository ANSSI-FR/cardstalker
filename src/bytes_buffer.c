#include "bytes_buffer.h"



BUFF_Status BUFF_Init(BUFF_Buffer *pBuffer){
	if(pBuffer == NULL){
		return BUFF_ERR;
	}
	
	pBuffer->currentSize = 0;
	pBuffer->readIndex = 0;
	pBuffer->writeIndex = 0;
	
	return BUFF_OK;
}


BUFF_Status BUFF_IsEmpty(BUFF_Buffer *pBuffer){
	if(pBuffer == NULL){
		return BUFF_ERR;
	}
	
	if(((pBuffer->readIndex) == (pBuffer->writeIndex)) && (BUFF_IsFull(pBuffer) == BUFF_NO)){
		return BUFF_OK;
	}
	else{
		return BUFF_NO;
	}
}


BUFF_Status BUFF_IsFull(BUFF_Buffer *pBuffer){
	if(pBuffer == NULL){
		return BUFF_ERR;
	}
	
	if((pBuffer->currentSize) == BUFF_MAX_SIZE){
		return BUFF_OK;
	}
	else{
		return BUFF_NO;
	}
}


BUFF_Status BUFF_Enqueue(BUFF_Buffer *pBuffer, uint8_t byte){
	if(pBuffer == NULL){
		return BUFF_ERR;
	}
	
	if(BUFF_IsFull(pBuffer) == BUFF_OK){
		return BUFF_FULL;
	}
	
	pBuffer->array[pBuffer->writeIndex] = byte;
	pBuffer->writeIndex = (pBuffer->writeIndex + 1) % BUFF_MAX_SIZE;
	pBuffer->currentSize = pBuffer->currentSize + 1;
	
	return BUFF_OK;
}


BUFF_Status BUFF_Dequeue(BUFF_Buffer *pBuffer, uint8_t *pByte){
	if(pBuffer == NULL){
		return BUFF_ERR;
	}
	
	if(BUFF_IsEmpty(pBuffer) == BUFF_OK){
		return BUFF_EMPTY;
	}
	
	*pByte = pBuffer->array[pBuffer->readIndex];
	pBuffer->readIndex = (pBuffer->readIndex + 1) % BUFF_MAX_SIZE;
	pBuffer->currentSize = pBuffer->currentSize - 1;
	
	return BUFF_OK;
}


BUFF_Status BUFF_GetCurrentSize(BUFF_Buffer *pBuffer, uint32_t *pCurrentSize){
	if(pBuffer == NULL){
		return BUFF_ERR;
	}
	
	*pCurrentSize = pBuffer->currentSize;
	
	return BUFF_OK;
}


BUFF_Status BUFF_EmptyIt(BUFF_Buffer *pBuffer){
	BUFF_Status rv;
	
	if(pBuffer == NULL){
		return BUFF_ERR;
	}
	
	rv = BUFF_Init(pBuffer);
	if(rv != BUFF_OK) return BUFF_ERR;
	
	
	return BUFF_OK;
}


BUFF_Status BUFF_Move(BUFF_Buffer *pBuffDest, BUFF_Buffer *pBuffSrc){
	BUFF_Status rv, rv2;
	uint8_t byte;
	
	
	if(pBuffDest == NULL){
		return BUFF_ERR;
	}
	
	if(pBuffSrc == NULL){
		return BUFF_OK;
	}
	
	
	while((rv = BUFF_Dequeue(pBuffSrc, &byte)) == BUFF_OK){
		rv2 = BUFF_Enqueue(pBuffDest, byte);
		if(rv2 != BUFF_OK) return BUFF_ERR;
	}
	
	if(rv != BUFF_EMPTY) return BUFF_ERR;
	
	
	return BUFF_OK;
}


/**
 * \fn BUFF_Status BUFF_Copy(BUFF_Buffer *pBuffDest, BUFF_Buffer *pBuffSrc)
 * \brief Copy a #BUFF_Buffer structure into another.
 * \param *pBuffDest is a pointer on the destination #BUFF_Buffer structure.
 * \param *pBuffSrc is a pointer on the source #BUFF_Buffer structure.
 * \return This function returns a #BUFF_Status code which indicates if the function behaved as expected or not.
 * 
 * This function copy a source #BUFF_Buffer struct into a destination #BUFF_Buffer struct.
 * The data and teh current state are preserved. 
 */
BUFF_Status BUFF_Copy(BUFF_Buffer *pBuffDest, const BUFF_Buffer *pBuffSrc){
	uint32_t i;
	
	
	if(pBuffDest == NULL){
		return BUFF_ERR;
	}
	
	if(pBuffSrc == NULL){  /* Nothing to do  */
		return BUFF_OK;
	}
	
	
	for(i=0; i<BUFF_MAX_SIZE; i++){
		pBuffDest->array[i] = pBuffSrc->array[i];
	}
	
	
	pBuffDest->currentSize = pBuffSrc->currentSize;
	pBuffDest->readIndex = pBuffSrc->readIndex;
	pBuffDest->writeIndex = pBuffSrc->writeIndex;
	
	
	return BUFF_OK;
}

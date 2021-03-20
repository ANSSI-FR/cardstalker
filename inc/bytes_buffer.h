/**
 * \file bytes_buffer.h
 * \copyright This file is part of the Card-Stalker project and is distributed under the GPLv3 license. See LICENSE file in the root directory of the project.
 */


#ifndef __BYTES_BUFFER_H__
#define __BYTES_BUFFER_H__



#include <stdint.h>
#include <stddef.h>



/**
* \def BUFF_MAX_SIZE
* BUFF_MAX_SIZE defines the maximum size (in bytes) of the static buffer in the BUFF_Status struct. 
*/
#define BUFF_MAX_SIZE ((uint32_t)(1000))




/**
 * \enum BUFF_Status
 * This type is used to encode the returned execution code of all the functions interacting with the bytes buffer.
 * For each function, this code indicates if the function behaved as expected or not.
 */
typedef enum BUFF_Status BUFF_Status;
enum BUFF_Status{
	BUFF_OK                       = (uint32_t)(0x00000001),
	BUFF_NO                       = (uint32_t)(0x00000002),
	BUFF_FULL                     = (uint32_t)(0x00000003),
	BUFF_EMPTY                    = (uint32_t)(0x00000004),
	BUFF_ERR                      = (uint32_t)(0x00000000)
};



/**
 * \struct READER_T1_Block
 * This struture represents a Block object as described in ISO7816-3 section 11.3.1. 
 */
typedef struct BUFF_Buffer BUFF_Buffer;
struct BUFF_Buffer{
	uint8_t array[BUFF_MAX_SIZE];   /*!< Static array storing the elements.                                            */
	uint32_t currentSize;           /*!< Number of elements currently filled in the buffer.                            */
	uint32_t readIndex;             /*!< Integer index pointing on the next element to be read in the raw array.       */
	uint32_t writeIndex;            /*!< Integer index pointing on the next element to be written in the raw array.    */
};




BUFF_Status BUFF_Init(BUFF_Buffer *pBuffer);
BUFF_Status BUFF_IsEmpty(BUFF_Buffer *pBuffer);
BUFF_Status BUFF_IsFull(BUFF_Buffer *pBuffer);
BUFF_Status BUFF_Enqueue(BUFF_Buffer *pBuffer, uint8_t byte);
BUFF_Status BUFF_Dequeue(BUFF_Buffer *pBuffer, uint8_t *pByte);
BUFF_Status BUFF_GetCurrentSize(BUFF_Buffer *pBuffer, uint32_t *pCurrentSize);
BUFF_Status BUFF_EmptyIt(BUFF_Buffer *pBuffer);
BUFF_Status BUFF_Move(BUFF_Buffer *pBuffDest, BUFF_Buffer *pBuffSrc);
BUFF_Status BUFF_Copy(BUFF_Buffer *pBuffDest, const BUFF_Buffer *pBuffSrc);



#endif

/**
 * \file semaphore.h
 * \copyright This file is part of the Card-Stalker project and is distributed under the GPLv3 license. See LICENSE file in the root directory of the project.
 * This file provides the necessary definitions for the semaphore library.
 * This librairy provides synchronisation tools between concurrent processes based on the Disjkstra definition.
 */


#ifndef __STATE_SEMAPHORE_H__
#define __STATE_SEMAPHORE_H__


#include <stdint.h>



/**
 * \enum SEM_Status
 * This type is used to encode the returned execution code of all the functions interacting with the semaphore.
 * For each function, this return code indicates if the function behaved as expected or not.
 */
typedef enum SEM_Status SEM_Status;
enum SEM_Status{
	SEM_OK                       = (uint32_t)(0x00000001),
	SEM_NO                       = (uint32_t)(0x00000002),
	SEM_LOCKED                   = (uint32_t)(0x00000003),
	SEM_UNLOCKED                 = (uint32_t)(0x00000004),
	SEM_ERR                      = (uint32_t)(0x00000000)
};


/**
 * \struct SEM_Handle
 * This structure contains all the informations necessary to define a semaphore object.
 */
typedef struct SEM_Handle SEM_Handle;
struct SEM_Handle{
	uint32_t counter;                  /*!< Current counter.     */
};



SEM_Status SEM_Init(SEM_Handle *pSem, uint32_t capacity);
SEM_Status SEM_Lock(SEM_Handle *pSem);
SEM_Status SEM_TryLock(SEM_Handle *pSem);
SEM_Status SEM_IsLocked(SEM_Handle *pSem);
SEM_Status SEM_Release(SEM_Handle *pSem);

SEM_Status SEM_DelayCallback(SEM_Handle *pSem);


#endif

/**
 * \file semaphore.c
 * \copyright This file is part of the Card-Stalker project and is distributed under the GPLv3 license. See LICENSE file in the root directory of the project.
 * This file provides a semaphore/mutex service with the corresponding data structures and methods to operate on.
 */


#include "semaphore.h"



/**
 * \fn SEM_Status SEM_Init(SEM_Handle *pSem, uint32_t capacity)
 * \brief Initializes the semaphore handle struct.
 * \return This function returns a SEM_Status execution code.
 * \param *pSem is a pointer on the semaphore handle struct.
 * \param capacity is a positive integer used to define the capacity of the semaphore to be created.
 */
SEM_Status SEM_Init(SEM_Handle *pSem, uint32_t capacity){
	pSem->counter = capacity;
	
	return SEM_OK;
}


/**
 * \fn SEM_Status SEM_Lock(SEM_Handle *pSem)
 * \brief Semaphore lock function.
 * \return This function returns a SEM_Status execution code.
 * \param *pSem is a pointer on the semaphore handle struct.
 * 
 * This function is used to lock the semaphore. This function waits untill the semaphore is available and then locks it.
 * The user can re-define the #SEM_DelayCallback() function to specify a delay/sleep function.
 * By default the function is looping while the semaphore is still locked.
 */
SEM_Status SEM_Lock(SEM_Handle *pSem){
	SEM_Status rv;
	
	while((pSem->counter) == 0){
		rv = SEM_DelayCallback(pSem);
		if(rv != SEM_OK) return SEM_ERR;
	}
	
	pSem->counter--;
	
	return SEM_OK;
}


/**
 * \fn SEM_Status SEM_TryLock(SEM_Handle *pSem)
 * \brief Semaphore try-lock function.
 * \return This function returns a SEM_Status code. SEM_LOCKED indicates that is already in a locked state. 
 * SEM_UNLOCKED indicates that the semaphore was not locked and the function managed to lock it.
 * Any other value indicates an error.
 * \param *pSem is a pointer on the semaphore handle struct.
 * 
 * This function is used to test wether the semaphore is in a locked state or not.
 * If the semaphore is not locked then we lock it and we return SEM_UNLOCKED.
 * If the semaphore is already locked, then we exit the function by returning SEM_LOCKED.
 */
SEM_Status SEM_TryLock(SEM_Handle *pSem){
	SEM_Status rv;
	
	if((pSem->counter) == 0){
		return SEM_LOCKED;
	}
	else{
		rv = SEM_Lock(pSem);
		if(rv != SEM_OK) return SEM_ERR;
		return SEM_UNLOCKED;
	}
}


/**
 * \fn SEM_Status SEM_IsLocked(SEM_Handle *pSem)
 * \brief Check if semaphore is in a locked state.
 * \return This function returns a SEM_Status code. SEM_LOCKED indicates that is already in a locked state. SEM_UNLOCKED indicated that the semaphore is currently not locked.
 * \param *pSem is a pointer on the semaphore handle struct.
 * 
 * This function is used to test wether the semaphore is in a locked state or not.
 */
SEM_Status SEM_IsLocked(SEM_Handle *pSem){
	if((pSem->counter) == 0){
		return SEM_LOCKED;
	}
	else{
		return SEM_UNLOCKED;
	}
}


/**
 * \fn SEM_Status SEM_Release(SEM_Handle *pSem)
 * \brief Semaphore release function.
 * \return This function returns a SEM_Status execution code.
 * \param *pSem is a pointer on the semaphore handle struct.
 * 
 * This function is used to release the semaphore.
 */
SEM_Status SEM_Release(SEM_Handle *pSem){
	pSem->counter++;
	
	return SEM_OK;
}


/**
 * \fn SEM_Status SEM_DelayCallback(SEM_Handle *pSem)
 * \brief User re-defined function to generate a delay.
 * \return This function returns a SEM_Status execution code.
 * \param *pSem is a pointer on the semaphore handle struct.
 * 
 * This function has to be eventually re-defined by the user.
 * The purpose of this function is to generate a delay/sleep function used when waiting for the semaphore to be unlocked when using the #SEM_Lock() function.
 */
__attribute__((weak)) SEM_Status SEM_DelayCallback(SEM_Handle *pSem){
	return SEM_OK;
}

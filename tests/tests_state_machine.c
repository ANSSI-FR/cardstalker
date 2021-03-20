#include "unity.h"

#include "state_machine.h"
#include "tests_state_machine.h"






#ifdef TEST



uint32_t globalFlagBlockSent;
uint32_t globalFlagBlockReceived;
uint32_t globalFlagTxe;
uint32_t globalFlagRxne;
uint32_t globalFlagAckCallback;


void setUp(void){
	globalFlagBlockSent = 0;
	globalFlagBlockReceived = 0;
	globalFlagAckCallback = 0;
}


void tearDown(void){
	
}



int main(){
	UNITY_BEGIN();
	
	RUN_TEST(test_SM_ReceiveDataBlockShouldWork);
	RUN_TEST(test_SM_ReceiveEmptyDataBlockShouldWork);
	RUN_TEST(test_SM_ReceiveControlBlockShouldWork);
	RUN_TEST(test_SM_TwoReceiveInARow);
	RUN_TEST(test_SM_SendDataBlockShouldWork);
	RUN_TEST(test_SM_TwoSendInARow);
	RUN_TEST(test_SM_SendEmptyDataBlockShouldWork);
	RUN_TEST(test_SM_SendDataBlockWhenRcvOngoing_case01);
	RUN_TEST(test_SM_SendDataBlockWhenRcvOngoing_case02);
	RUN_TEST(test_SM_SendControlBlockShouldWork);
	RUN_TEST(test_SM_SendDataBlock_anotherBlockInsteadAck);
	RUN_TEST(test_SM_SerializationOfDataBlockSendCalls);
	RUN_TEST(test_SM_SerializationOfNonAckedBlockSendCalls);
	RUN_TEST(test_SM_shouldNotWaitForAckAfterBusyBlock);
	RUN_TEST(test_SM_rcvMutexShouldWork);
	RUN_TEST(test_SM_SendMutexShouldWork);
	RUN_TEST(test_SM_ExtraParasitByteReceived);
	RUN_TEST(test_SM_checkAckIsSentAfterDataRcpt);
	RUN_TEST(test_SM_checkAckIsSentAfterCtrlRcpt);
	RUN_TEST(test_SM_checkAckIsCorrectlyQueued);
	RUN_TEST(test_SM_EvolveStateOnByteTransmission_shouldResetTxeWhenDone);
	RUN_TEST(test_SM_ACK_BLOCK_ReceivedCallback_Case01);
	RUN_TEST(test_SM_ACK_BLOCK_ReceivedCallback_Case02);
	RUN_TEST(test_SM_ACK_BLOCK_ReceivedCallback_Case03);
	
	return UNITY_END();
}
#endif



SM_Status SM_ACK_BLOCK_ReceivedCallback(SM_Handle *pHandle){
	globalFlagAckCallback = 1;
	
	return SM_OK;
}




SM_Status SM_BlockSentCallback(SM_Handle *pHandle){
	globalFlagBlockSent = 1;
	
	return SM_OK;
}


SM_Status SM_BlockRecievedCallback(SM_Handle *pHandle){
	globalFlagBlockReceived = 1;
	
	return SM_OK;
}



SM_Status SM_EnableRxneInterrupt_Callback(SM_Handle *pHandle){
	globalFlagRxne = 1;
	
	return SM_OK;
}


SM_Status SM_DisableRxneInterrupt_Callback(SM_Handle *pHandle){
	globalFlagRxne = 0;
	
	return SM_OK;
}


SM_Status SM_EnableTxeInterrupt_Callback(SM_Handle *pHandle){
	globalFlagTxe = 1;
	
	return SM_OK;
}


SM_Status SM_DisableTxeInterrupt_Callback(SM_Handle *pHandle){
	globalFlagTxe = 0;
	
	return SM_OK;
}



void test_SM_ReceiveDataBlockShouldWork(void){
	SM_Status rv;
	SM_Handle handle;
	BUFF_Buffer *pBuffer, rcvBuffer;
	BUFF_Status buffRv;
	uint8_t byte;
	
	
	/* Initiating block reception ...  */
	rv  = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcvBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_DATA_BLOCK);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xAB);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xCD);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Reception process should not be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 0);
	
	
	/* We check that the ACK is sent ...  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* Reception process should be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 1);
	
	
	/* Checking the received block ...  */
	rv = SM_GetRcptBufferPtr(&handle, &pBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	buffRv = BUFF_Dequeue(pBuffer, &byte);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8(0xAB, byte);
	
	buffRv = BUFF_Dequeue(pBuffer, &byte);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8(0xCD, byte);
	
	buffRv = BUFF_IsEmpty(pBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	rv = SM_IsAllDataRecieved(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_IsDataAvail(&handle);
	TEST_ASSERT_TRUE(rv == SM_NO);
	
}



void test_SM_ReceiveEmptyDataBlockShouldWork(void){
	SM_Status rv;
	SM_Handle handle;
	BUFF_Buffer *pBuffer, rcvBuffer;
	BUFF_Status buffRv;
	uint8_t byte;
	
	
	/* Initiating block reception ...  */
	rv  = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcvBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_DATA_BLOCK);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Reception process should not be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 0);
	
	
	/* We check that the ACK is sent ...  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* Reception process should be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 1);
	
	
	/* Checking the received block ...  */
	rv = SM_GetRcptBufferPtr(&handle, &pBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	buffRv = BUFF_IsEmpty(pBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	rv = SM_IsAllDataRecieved(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_IsDataAvail(&handle);
	TEST_ASSERT_TRUE(rv == SM_NO);
	
}





void test_SM_ReceiveControlBlockShouldWork(void){
	SM_Status rv;
	SM_Handle handle;
	BUFF_Buffer *pBuffer, rcptBuffer;
	BUFF_Status buffRv;
	uint8_t byte;
	
	
	/* Initiating block reception ...  */
	rv  = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcptBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_COLD_RST_BLOCK);  /* Control block = Cold reset */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	
	/* Reception process should not be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 0);
	
	
	/* We check that the ACK is sent ...  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* Reception process should be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 1);
	
	
	/* Checking the received block ...  */
	rv = SM_IsAllDataRecieved(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_IsDataAvail(&handle);
	TEST_ASSERT_TRUE(rv == SM_NO);
	
	TEST_ASSERT_EQUAL_UINT32(0, handle.rcvHandle.nbDataExpected);
	TEST_ASSERT_EQUAL_UINT32(0, handle.rcvHandle.nbDataRcvd);

}



void test_SM_TwoReceiveInARow(void){
	SM_Status rv;
	SM_Handle handle;
	BUFF_Buffer *pBuffer, rcptBuffer;
	BUFF_Status buffRv;
	uint8_t byte;
	
	
	/* Initiating block reception ...  */
	rv  = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcptBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_COLD_RST_BLOCK);  /* Control block = Cold reset */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Reception process should not be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 0);
	
	
	/* We check that the ACK is sent ...  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* Reception process should be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 1);
	
	
	/* Checking the received block ...  */
	rv = SM_IsAllDataRecieved(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_IsDataAvail(&handle);
	TEST_ASSERT_TRUE(rv == SM_NO);
	
	
	TEST_ASSERT_EQUAL_UINT32(0, handle.rcvHandle.nbDataExpected);
	TEST_ASSERT_EQUAL_UINT32(0, handle.rcvHandle.nbDataRcvd);
	
	
	
	/* Receiving another block ... */
	globalFlagBlockReceived = 0;
	
	rv = SM_ReceiveBlock(&handle, &rcptBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_DATA_BLOCK);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xAB);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xCD);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Reception process should not be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 0);
	
	
	/* We check that the ACK is sent ...  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* Reception process should be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 1);
	
	
	/* Checking the received block ...  */
	rv = SM_GetRcptBufferPtr(&handle, &pBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	buffRv = BUFF_Dequeue(pBuffer, &byte);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8(0xAB, byte);
	
	buffRv = BUFF_Dequeue(pBuffer, &byte);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8(0xCD, byte);
	
	buffRv = BUFF_IsEmpty(pBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	rv = SM_IsAllDataRecieved(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_IsDataAvail(&handle);
	TEST_ASSERT_TRUE(rv == SM_NO);

}


/* We do not start a new block reception until SM_ReceiveBlock() is not called again ...  */
void test_SM_ExtraParasitByteReceived(void){
	SM_Status rv;
	SM_Handle handle;
	BUFF_Buffer *pBuffer, rcptBuffer;
	BUFF_Status buffRv;
	uint8_t byte;
	
	
	/* Initiating block reception ...  */
	rv  = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcptBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_DATA_BLOCK);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xAB);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xCD);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xEE);  /* Extra unexpected byte, this byte should not be processed.  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Checking the received block ...  */
	rv = SM_GetRcptBufferPtr(&handle, &pBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	buffRv = BUFF_Dequeue(pBuffer, &byte);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8(0xAB, byte);
	
	buffRv = BUFF_Dequeue(pBuffer, &byte);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8(0xCD, byte);
	
	buffRv = BUFF_IsEmpty(pBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	rv = SM_IsAllDataRecieved(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_IsDataAvail(&handle);
	TEST_ASSERT_TRUE(rv == SM_NO);

}






void test_SM_SendDataBlockShouldWork(void){
	BUFF_Buffer dataBuffer;
	BUFF_Status buffRv;
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'e');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'s');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	/* Sending the data ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_DATA_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x04, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'e', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'s', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 4 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	/* We check that the transmit process is not over ...    */
	/* We still have to wait for an ACK ...                  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	
	/* We emulate the reception of an ACK block + LRC ...          */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* We check that the block transmission is over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 1);
	
}



void test_SM_TwoSendInARow(void){
	BUFF_Buffer dataBuffer;
	BUFF_Status buffRv;
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'e');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'s');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	/* Sending the data ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_DATA_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x04, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'e', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'s', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 4 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	/* We check that the transmit process is not over ...    */
	/* We still have to wait for an ACK ...                  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	
	/* We emulate the reception of an ACK block + LRC ...          */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* We check that the block transmission is over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 1);
	
	
	
	/* We do the sencond transmission ...  */
	globalFlagBlockSent = 0;
	
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'A');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'B');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'C');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_DATA_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x03, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'A', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'B', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'C', byte);
	
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	/* We check that the transmit process is not over ...    */
	/* We still have to wait for an ACK ...                  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	
	/* We emulate the reception of an ACK block + LRC ...          */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* We check that the block transmission is over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 1);
	
}




void test_SM_SendEmptyDataBlockShouldWork(void){
	BUFF_Buffer dataBuffer;
	BUFF_Status buffRv;
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	/* Sending the data ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_DATA_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	/* We check that the transmit process is not over ...    */
	/* We still have to wait for an ACK ...                  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	
	/* We emulate the reception of an ACK block + LRC ...          */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* We check that the block transmission is over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 1);
	
}




void test_SM_SendDataBlockWhenRcvOngoing_case01(void){
	BUFF_Buffer dataBuffer, *pBuffer, rcptBuffer;
	BUFF_Status buffRv;
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'e');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'s');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	/* Sending the data and starting a concurrent reception ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcptBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	
	/* Emulating some received bytes ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_DATA_BLOCK);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xAB);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xCD);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_DATA_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x04, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'e', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'s', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 4 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* We check that the transmit process is not over ...    */
	/* We still have to wait for an ACK ...                  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	
	/* We check that the receive process is not over ...    */
	/* We still have to wait to send an ACK ...             */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 0);	

	
	
	/* Here, the transmission state machine should change its path and instead of waiting for ACK it should take time to send ACK right now ...  */
	/* we check transmission of ACK + LRC ..  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control Block = ACK */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* The reception process should be over now ...  */
	/* Checking the received block ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 1);	
	
	/* Transmission process should still not be over ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	
	
	/* We emulate the reception of an ACK block + LRC ...          */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);   /* Control Block = ACK */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);           /* LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	
	/* We check that the block transmission is over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 1);	
	
}






void test_SM_SendDataBlockWhenRcvOngoing_case02(void){
	BUFF_Buffer dataBuffer, *pBuffer, rcptBuffer;
	BUFF_Status buffRv;
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'e');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'s');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	/* Sending the data and starting a concurrent reception ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcptBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	
	/* Emulating some received bytes ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_DATA_BLOCK);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_DATA_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x04, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'e', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'s', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 4 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* We check that the transmit process is not over ...    */
	/* We still have to wait for an ACK ...                  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	
	/* Emulating some received bytes ...  */
	rv = SM_EvolveStateOnByteReception(&handle, 0xAB);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xCD);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* We check that the receive process is not over ...    */
	/* We still have to wait to send an ACK ...             */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 0);	
	
	
	
	/* We emulate the reception of an ACK block + LRC ...          */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);   /* Control Block = ACK */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);           /* LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	
	/* We check that the block transmission is over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 1);	

	
	
	/* Here, the transmission state machine should change its path and instead of waiting for ACK it should take time to send ACK right now ...  */
	/* we check transmission of ACK + LRC ..  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control Block = ACK */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* The reception process should be over now ...  */
	/* Checking the received block ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 1);	
	
	
}





void test_SM_SendControlBlockShouldWork(void){
	BUFF_Buffer dataBuffer;
	BUFF_Status buffRv;
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	
	/* Sending the data ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, NULL, SM_COLD_RST_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte = Cold reset */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_COLD_RST_BLOCK, byte);
	
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* We check that the transmit process is not over ...    */
	/* We still have to wait for an ACK ...                  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	
	/* We emulate the reception of an ACK block + LRC ...          */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);   /* Control block = ACK */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);           /* LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* We check that the block transmission is over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 1);
}


void test_SM_rcvMutexShouldWork(void){
	SM_Status rv;
	SM_Handle handle;
	BUFF_Buffer *pBuffer, rcptBuffer;
	BUFF_Status buffRv;
	SEM_Status semRv;
	uint8_t byte;
	
	
	/* Initiating block reception ...  */
	rv  = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcptBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* We artificially simulate a locked mutex, we request a lock from outside ... */
	semRv = SEM_Lock(&(handle.rcvHandle.contextAccessMutex));
	TEST_ASSERT_TRUE(semRv == SEM_OK);
	
	/* ... So the following received byte should not be processed */
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_BUSY);
	
	/* We check that LEN 2 has not been processed ...  */
	TEST_ASSERT_TRUE(handle.rcvHandle.currentState == SM_RCVSTATE_LEN_BYTE1);
	
	/* We artificially request to release the mutex from the outside ... */
	semRv = SEM_Release(&(handle.rcvHandle.contextAccessMutex));
	TEST_ASSERT_TRUE(semRv == SEM_OK);
	
	///* A BUSY block should have been send from the bridge to the computer, we check that  ...  */
	//rv = SM_EvolveStateOnByteTransmission(&handle, &byte);
	//TEST_ASSERT_TRUE(rv == SM_OK);
	//TEST_ASSERT_EQUAL_UINT8(SM_BUSY_BLOCK, byte);
	
	
	/* The computer re sends the LEN 2 byte ...                                         */
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xAB);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xCD);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	
	/* Reception process should not be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 0);
	
	
	/* We check that the ACK is sent ...  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);  
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* Reception process should be over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockReceived == 1);
	
	
	/* Checking the received block ...  */
	rv = SM_GetRcptBufferPtr(&handle, &pBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	buffRv = BUFF_Dequeue(pBuffer, &byte);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8(0xAB, byte);
	
	buffRv = BUFF_Dequeue(pBuffer, &byte);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8(0xCD, byte);
	
	buffRv = BUFF_IsEmpty(pBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	rv = SM_IsAllDataRecieved(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_IsDataAvail(&handle);
	TEST_ASSERT_TRUE(rv == SM_NO);
	
}





void test_SM_SendMutexShouldWork(void){
	BUFF_Buffer dataBuffer;
	BUFF_Status buffRv;
	SEM_Status semRv;
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'e');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'s');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	/* Sending the data ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x04, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'e', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'s', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 4 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	/* We artificially lock the mutex and we try again ...  */
	semRv = SEM_Lock(&(handle.sendHandle.contextAccessMutex));
	TEST_ASSERT_TRUE(semRv == SEM_OK);
	
	/* The following byte should not be processed ...  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_BUSY);
	//TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* We check that CHECK has not been processed (se still should be in the previous state) ...  */
	TEST_ASSERT_TRUE(handle.sendHandle.currentState == SM_SENDSTATE_DATA);
	
	/* We artificially request to release the mutex from the outside ... */
	semRv = SEM_Release(&(handle.sendHandle.contextAccessMutex));
	TEST_ASSERT_TRUE(semRv == SEM_OK);
	
	
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	TEST_ASSERT_TRUE(handle.sendHandle.currentState == SM_SENDSTATE_CHECK);
	
	
	
	/* We check that the transmit process is not over ...    */
	/* We still have to wait for an ACK ...                  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	
	/* We emulate the reception of an ACK block + LRC ...          */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* We check that the block transmission is over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 1);
	
}




void test_SM_checkAckIsSentAfterDataRcpt(void){
	SM_Status rv;
	SM_Handle handle;
	BUFF_Buffer *pBuffer, rcptBuffer;
	BUFF_Status buffRv;
	uint8_t byte;
	
	
	/* Initiating block reception ...  */
	rv  = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcptBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x01);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xAB);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Checking the data being sent ...  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* ACK ctrl byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* ACK LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
}



void test_SM_checkAckIsSentAfterCtrlRcpt(void){
	SM_Status rv;
	SM_Handle handle;
	BUFF_Buffer *pBuffer, rcptBuffer;
	BUFF_Status buffRv;
	uint8_t byte;
	
	
	/* Initiating block reception ...  */
	rv  = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcptBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_COLD_RST_BLOCK);  /* Control block = Cold reset */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Checking the data being sent ...  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* ACK ctrl byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* ACK LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
}



void test_SM_checkAckIsCorrectlyQueued(void){
	SM_Status rv;
	SM_Handle handle;
	BUFF_Buffer *pBuffer, dataBuffer, rcptBuffer;
	BUFF_Status buffRv;
	uint8_t byte;
	
	
	/* We are in the case where the bridge receives data from the computer while it is sending data to the computer at the same time. */
	/* We want to make sure that the ACK will be sent right after the data being currently transmitted ...                            */
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'e');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'s');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	/* Initiating block reception and transmission ...  */
	rv  = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcptBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x01);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xAB);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Checking transmitted bytes ...  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x04, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'e', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'s', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 4 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* Checking that the ACK block is sent ...  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* ACK ctrl byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* ACK LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
}




void test_SM_SerializationOfDataBlockSendCalls(void){
	/* When we send a data block it should not be possible to make another call to SM_BlockSend() while the ACK is not received */
	BUFF_Buffer dataBuffer;
	BUFF_Status buffRv;
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'e');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'s');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	/* Sending the data ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* Trying another call to SM_SendBlock() ...  */
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_BUSY);
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_DATA_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x04, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'e', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'s', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 4 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* Trying another call to SM_SendBlock() ...                  */
	/* Should not work because the ACK is still not received ...  */
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_BUSY);
	
	
	/* We emulate the reception of an ACK block + LRC ...          */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* Trying another call to SM_SendBlock() ...  */
	/* Should work this time ...                  */
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
}



void test_SM_SerializationOfNonAckedBlockSendCalls(void){
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	
	/* Sending the data ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, NULL, SM_BUSY_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* Trying another call to SM_SendBlock() ...  */
	rv = SM_SendBlock(&handle, NULL, SM_BUSY_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_BUSY);
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte  = BUSY */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_BUSY_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* Trying another call to SM_SendBlock() ...  */
	rv = SM_SendBlock(&handle, NULL, SM_BUSY_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
}



void test_SM_shouldNotWaitForAckAfterBusyBlock(void){
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	
	/* Sending the data ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, NULL, SM_BUSY_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte  = BUSY */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_BUSY_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* We check that the block transmission is over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 1);
}



void test_SM_SendDataBlock_anotherBlockInsteadAck(void){
	BUFF_Buffer dataBuffer;
	BUFF_Status buffRv;
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'e');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'s');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	/* Sending the data ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_DATA_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x04, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'e', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'s', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 4 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	/* We check that the transmit process is not over ...    */
	/* We still have to wait for an ACK ...                  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	
	/* We emulate the reception of COLD RESET block + LRC instead of an ACK  ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_COLD_RST_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* We check that the block transmission is not over ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
}




void test_SM_EvolveStateOnByteTransmission_shouldResetTxeWhenDone(void){
	BUFF_Buffer dataBuffer;
	BUFF_Status buffRv;
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'e');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'s');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	/* Sending the data ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	TEST_ASSERT_TRUE(globalFlagTxe == 0);
	
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	TEST_ASSERT_TRUE(globalFlagTxe == 1);
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_DATA_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x04, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'e', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'s', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 4 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	/* We check that the transmit process is not over ...    */
	/* We still have to wait for an ACK ...                  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 0);
	
	
	TEST_ASSERT_TRUE(globalFlagTxe == 0);
	
	
	/* We emulate the reception of an ACK block + LRC ...          */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* We check that the block transmission is over now ...  */
	TEST_ASSERT_TRUE(globalFlagBlockSent == 1);
	
	TEST_ASSERT_TRUE(globalFlagTxe == 0);	
}


void test_SM_ACK_BLOCK_ReceivedCallback_Case01(void){
	SM_Status rv;
	SM_Handle handle;
	BUFF_Buffer *pBuffer, rcvBuffer;
	BUFF_Status buffRv;
	uint8_t byte;
	
	
	/* In this test case we dont want the ACK callback to be called because the currently received ACK was not expected ...  */
	
	
	/* Initiating block reception ...  */
	rv  = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcvBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* We check that the ACK callback has not been called yet ...  */
	TEST_ASSERT_TRUE(globalFlagAckCallback == 0);
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	/* We check that the ACK callback has not been called ...  */
	TEST_ASSERT_TRUE(globalFlagAckCallback == 0);
}



void test_SM_ACK_BLOCK_ReceivedCallback_Case02(void){
	SM_Status rv;
	SM_Handle handle;
	BUFF_Buffer *pBuffer, rcvBuffer;
	BUFF_Status buffRv;
	uint8_t byte;
	
	
	/* Initiating block transmission ...  */
	rv  = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	rv = SM_SendBlock(&handle, NULL, SM_COLD_RST_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte = Cold reset */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_COLD_RST_BLOCK, byte);
	
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* check LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	
	/* We check that the ACK callback has not been called yet ...  */
	TEST_ASSERT_TRUE(globalFlagAckCallback == 0);
	
	
	/* Emulating byte receptions ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* We check that the ACK callback has been called ...  */
	TEST_ASSERT_TRUE(globalFlagAckCallback == 1);
}



void test_SM_ACK_BLOCK_ReceivedCallback_Case03(void){
	BUFF_Buffer dataBuffer, *pBuffer, rcptBuffer;
	BUFF_Status buffRv;
	SM_Status rv;
	SM_Handle handle;
	uint8_t byte;
	
	
	/* Preparing test data to be sent ...  */
	buffRv = BUFF_Init(&dataBuffer);
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'e');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'s');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	buffRv = BUFF_Enqueue(&dataBuffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(buffRv == BUFF_OK);
	
	
	/* Sending the data and starting a concurrent reception ...  */
	rv = SM_Init(&handle);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_ReceiveBlock(&handle, &rcptBuffer);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_SendBlock(&handle, &dataBuffer, SM_DATA_BLOCK);
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	
	/* Emulating some received bytes ...  */
	rv = SM_EvolveStateOnByteReception(&handle, SM_DATA_BLOCK);  /* Control block */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	
	/* Checking the data being sent ... */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control byte */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_DATA_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LEN 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x04, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 1 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 2 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'e', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 3 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'s', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* data 4 */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);

	
	/* Emulating some received bytes ...  */
	rv = SM_EvolveStateOnByteReception(&handle, 0xAB);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0xCD);  /* DATA */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);  /* LRC  */
	TEST_ASSERT_TRUE(rv == SM_OK);

	
	
	/* We emulate the reception of an ACK block + LRC ...          */
	rv = SM_EvolveStateOnByteReception(&handle, SM_ACK_BLOCK);   /* Control Block = ACK */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	rv = SM_EvolveStateOnByteReception(&handle, 0x00);           /* LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	
	
	/* We check that the ACK callback has not been called yet ...  */
	TEST_ASSERT_TRUE(globalFlagAckCallback == 0);
	
	
	/* Here, the transmission state machine should change its path and instead of waiting for ACK it should take time to send ACK right now ...  */
	/* we check transmission of ACK + LRC ..  */
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* Control Block = ACK */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = SM_EvolveStateOnByteTransmission(&handle, &byte);   /* LRC */
	TEST_ASSERT_TRUE(rv == SM_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* We check that the ACK callback has been called ...  */
	TEST_ASSERT_TRUE(globalFlagAckCallback == 1);
}

#include "unity.h"

#include "tests_bridge_advanced.h"
#include "bridge_advanced.h"
#include "state_machine.h"


#include "mock_reader_hal_basis.h"
#include "mock_reader_hal_comm_settings.h"
#include "mock_reader_periph.h"
#include "mock_reader_hal.h"

#include "tests_toolbox.h"

#include "reader_lib.h"





#ifdef TEST


uint32_t globalFlagRxne;
uint32_t globalFlagTxe;



void setUp(void){
	
}


void tearDown(void){
	
}


int main(void){
	UNITY_BEGIN();
	
	RUN_TEST(test_BRIDGE2_dataBlockShouldWork_Case01);
	RUN_TEST(test_BRIDGE2_dataBlockShouldWork_Case02);
	RUN_TEST(test_BRIDGE2_dataBlockNoAnswerFromCard);
	RUN_TEST(test_BRIDGE2_coldReset);
	RUN_TEST(test_BRIDGE2_TwoProcessesInARow_Case01);
	
	return UNITY_END();
}
#endif





BRIDGE2_Status BRIDGE2_EnableTxeInterrupt_Callback(void){
	globalFlagTxe = 1;
	
	return BRIDGE2_OK;
}


BRIDGE2_Status BRIDGE2_DisableTxeInterrupt_Callback(void){
	globalFlagTxe = 0;
	
	return BRIDGE2_OK;
}


BRIDGE2_Status BRIDGE2_EnableRxneInterrupt_Callback(void){
	globalFlagRxne = 1;
	
	return BRIDGE2_OK;
}


BRIDGE2_Status BRIDGE2_DisableRxneInterrupt_Callback(void){
	globalFlagRxne = 0;
	
	return BRIDGE2_OK;
}





void test_BRIDGE2_dataBlockShouldWork_Case01(void){
	READER_HAL_CommSettings settings;
	READER_Status readerRv;
	BRIDGE2_Status rv;
	uint8_t byte;
	
	
	READER_HAL_InitWithDefaults_ExpectAnyArgsAndReturn(READER_OK);
	
	/* Initialization of the advanced bridge ...  */
	readerRv = READER_HAL_InitWithDefaults(&settings);
	TEST_ASSERT_TRUE(readerRv == READER_OK);
	
	rv = BRIDGE2_Init(&settings);
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_Run();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Setting up the expected behaviour from the reader side, preparing mocks ...  */
	uint8_t expectedSentFrame[] = {0xAB, 0xCD};
	set_expected_CharFrame(expectedSentFrame, 2);
	
	uint8_t rcvdBytesFromCard[] = {0x90, 0x00};
	emulate_RcvCharFrame(rcvdBytesFromCard, 2);
	
	READER_HAL_RcvChar_ExpectAnyArgsAndReturn(READER_TIMEOUT);
	
	
	/* Testing bridge behaviour ...  */
	/* Sending data block to the bridge ...  */
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* Control block = data block */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0xAB);  /* DATA 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0xCD);  /* DATA 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Data block is received, the bridge should answer an ACK to the computer ...  */
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CTRL BYTE */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* The data block should be processed by the bridge now ...                         */
	/* The bridge is expected to send the data to the card and get back the answer ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	
	/* We check that the bridge sends back to the computer the datas received from the card ...  */
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* Control block */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x02, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* DATA 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x90, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* DATA 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* The computer sends back an ACK ... */
	rv = BRIDGE2_ProcessRxneInterrupt(SM_ACK_BLOCK);  /* Control block = ACK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
}




void test_BRIDGE2_dataBlockShouldWork_Case02(void){
	READER_HAL_CommSettings settings;
	READER_Status readerRv;
	BRIDGE2_Status rv;
	uint8_t byte;
	
	
	READER_HAL_InitWithDefaults_ExpectAnyArgsAndReturn(READER_OK);
	
	/* Initialization of the advanced bridge ...  */
	readerRv = READER_HAL_InitWithDefaults(&settings);
	TEST_ASSERT_TRUE(readerRv == READER_OK);
	
	rv = BRIDGE2_Init(&settings);
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	/* We simulate timer interrupt when no data is ready ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_Run();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Setting up the expected behaviour from the reader side, preparing mocks ...  */
	uint8_t expectedSentFrame[] = {0xAB, 0xCD};
	set_expected_CharFrame(expectedSentFrame, 2);
	
	uint8_t rcvdBytesFromCard[] = {0x90, 0x00};
	emulate_RcvCharFrame(rcvdBytesFromCard, 2);
	
	READER_HAL_RcvChar_ExpectAnyArgsAndReturn(READER_TIMEOUT);
	
	
	/* We simulate timer interrupt when no data is ready ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Testing bridge behaviour ...  */
	/* Sending data block to the bridge ...  */
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* Control block = data block */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0xAB);  /* DATA 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* We simulate timer interrupt when no data is ready ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	rv = BRIDGE2_ProcessRxneInterrupt(0xCD);  /* DATA 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* We simulate timer interrupt when no data is ready ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Data block is received, the bridge should answer an ACK to the computer ...  */
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CTRL BYTE */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* The data block should be processed by the bridge now ...                         */
	/* The bridge is expected to send the data to the card and get back the answer ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	
	/* We check that the bridge sends back to the computer the datas received from the card ...  */
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* Control block */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* We simulate timer interrupt when no data is ready ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x02, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* DATA 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x90, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* DATA 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* The computer sends back an ACK ... */
	rv = BRIDGE2_ProcessRxneInterrupt(SM_ACK_BLOCK);  /* Control block = ACK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* We simulate timer interrupt when no data is ready ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
}





void test_BRIDGE2_dataBlockNoAnswerFromCard(void){
	READER_HAL_CommSettings settings;
	READER_Status readerRv;
	BRIDGE2_Status rv;
	uint8_t byte;
	
	
	READER_HAL_InitWithDefaults_ExpectAnyArgsAndReturn(READER_OK);
	
	/* Initialization of the advanced bridge ...  */
	readerRv = READER_HAL_InitWithDefaults(&settings);
	TEST_ASSERT_TRUE(readerRv == READER_OK);
	
	rv = BRIDGE2_Init(&settings);
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_Run();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Setting up the expected behaviour from the reader side, preparing mocks ...  */
	uint8_t expectedSentFrame[] = {0xAB, 0xCD};
	set_expected_CharFrame(expectedSentFrame, 2);
	
	/*We dont receive any bytes from card, only timeout ... */ 
	READER_HAL_RcvChar_ExpectAnyArgsAndReturn(READER_TIMEOUT);
	
	
	/* Testing bridge behaviour ...  */
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* Control block = data block */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0xAB);  /* DATA 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0xCD);  /* DATA 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Data block is received, the bridge should answer an ACK to the computer ...  */
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CTRL BYTE */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	
	/* The data block should be processed by the bridge now ...                         */
	/* The bridge is expected to send the data to the card and get back the answer ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	/* Should not be a problem if called two times ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* Control block */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* The computer sends back an ACK ... */
	rv = BRIDGE2_ProcessRxneInterrupt(SM_ACK_BLOCK);  /* Control block = ACK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
}




void test_BRIDGE2_coldReset(void){
	READER_HAL_CommSettings settings;
	READER_Status readerRv;
	BRIDGE2_Status rv;
	uint8_t byte;
	
	
	READER_HAL_InitWithDefaults_ExpectAnyArgsAndReturn(READER_OK);
	
	/* Initialization of the advanced bridge ...  */
	readerRv = READER_HAL_InitWithDefaults(&settings);
	TEST_ASSERT_TRUE(readerRv == READER_OK);
	
	rv = BRIDGE2_Init(&settings);
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_Run();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Setting up the expected behaviour from the reader side, preparing mocks ...  */
	READER_HAL_DoColdReset_ExpectAndReturn(READER_OK);
	
	
	/* Testing bridge behaviour ...  */
	rv = BRIDGE2_ProcessRxneInterrupt(SM_COLD_RST_BLOCK);  /* Control block = cold reset */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Should not be a problem to call it here ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Data block is received, the bridge should answer an ACK to the computer ...  */
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CTRL BYTE */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* The control block should be processed by the bridge now ...                         */
	/* The bridge is expected to send the data to the card and get back the answer ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
}




void test_BRIDGE2_TwoProcessesInARow_Case01(void){
	READER_HAL_CommSettings settings;
	READER_Status readerRv;
	BRIDGE2_Status rv;
	uint8_t byte;
	
	
	
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	READER_HAL_InitWithDefaults_ExpectAnyArgsAndReturn(READER_OK);
	
	/* Initialization of the advanced bridge ...  */
	readerRv = READER_HAL_InitWithDefaults(&settings);
	TEST_ASSERT_TRUE(readerRv == READER_OK);
	
	rv = BRIDGE2_Init(&settings);
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_Run();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Setting up the expected behaviour from the reader side, preparing mocks ...  */
	uint8_t expectedSentFrame[] = {0xAB, 0xCD};
	set_expected_CharFrame(expectedSentFrame, 2);
	
	uint8_t rcvdBytesFromCard[] = {0x90, 0x00};
	emulate_RcvCharFrame(rcvdBytesFromCard, 2);
	
	READER_HAL_RcvChar_ExpectAnyArgsAndReturn(READER_TIMEOUT);
	
	
	/* Testing bridge behaviour ...  */
	/* Sending data block to the bridge ...  */
	rv = BRIDGE2_ProcessRxneInterrupt(SM_DATA_BLOCK);  /* Control block = data block */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0xAB);  /* DATA 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0xCD);  /* DATA 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Data block is received, the bridge should answer an ACK to the computer ...  */
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CTRL BYTE */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* The data block should be processed by the bridge now ...                         */
	/* The bridge is expected to send the data to the card and get back the answer ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	
	/* We check that the bridge sends back to the computer the datas received from the card ...  */
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* Control block */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x02, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* DATA 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x90, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* DATA 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* The computer sends back an ACK ... */
	rv = BRIDGE2_ProcessRxneInterrupt(SM_ACK_BLOCK);  /* Control block = ACK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* We start the second process ...  */
	
	
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	/* Setting up the expected behaviour from the reader side, preparing mocks ...  */
	uint8_t expectedSentFrame2[] = {0xAB, 0xCD};
	set_expected_CharFrame(expectedSentFrame2, 2);
	
	uint8_t rcvdBytesFromCard2[] = {0x90, 0x00};
	emulate_RcvCharFrame(rcvdBytesFromCard2, 2);
	
	READER_HAL_RcvChar_ExpectAnyArgsAndReturn(READER_TIMEOUT);
	
	
	/* Sending data block to the bridge ...  */
	rv = BRIDGE2_ProcessRxneInterrupt(SM_DATA_BLOCK);  /* Control block = data block */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x02);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0xAB);  /* DATA 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0xCD);  /* DATA 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	/* Data block is received, the bridge should answer an ACK to the computer ...  */
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CTRL BYTE */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	/* The data block should be processed by the bridge now ...                         */
	/* The bridge is expected to send the data to the card and get back the answer ...  */
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	
	/* We check that the bridge sends back to the computer the datas received from the card ...  */
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* Control block */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 3 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x02, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* DATA 1 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x90, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* DATA 2 */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
	
	
	
	/* The computer sends back an ACK ... */
	rv = BRIDGE2_ProcessRxneInterrupt(SM_ACK_BLOCK);  /* Control block = ACK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
	
	
	
	rv = BRIDGE2_ProcessTimerInterrupt();
	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
}





//void test_BRIDGE2_StopShouldWaitEndOfBlock(void){
//	READER_HAL_CommSettings settings;
//	READER_Status readerRv;
//	BRIDGE2_Status rv;
//	uint8_t byte;
//	
//	
//	READER_HAL_InitWithDefaults_ExpectAnyArgsAndReturn(READER_OK);
//	
//	/* Initialization of the advanced bridge ...  */
//	readerRv = READER_HAL_InitWithDefaults(&settings);
//	TEST_ASSERT_TRUE(readerRv == READER_OK);
//	
//	rv = BRIDGE2_Init(&settings);
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	
//	rv = BRIDGE2_Run();
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	
//	
//	/* Setting up the expected behaviour from the reader side, preparing mocks ...  */
//	uint8_t expectedSentFrame[] = {0xAB, 0xCD};
//	set_expected_CharFrame(expectedSentFrame, 2);
//	
//	uint8_t rcvdBytesFromCard[] = {0x90, 0x00};
//	emulate_RcvCharFrame(rcvdBytesFromCard, 2);
//	
//	READER_HAL_RcvChar_ExpectAnyArgsAndReturn(READER_TIMEOUT);
//	
//	
//	/* Testing bridge behaviour ...  */
//	/* Sending data block to the bridge ...  */
//	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* Control block = data block */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	
//	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 1 */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	
//	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* LEN 2 */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	
//	rv = BRIDGE2_ProcessRxneInterrupt(0x02);  /* LEN 3 */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	
//	rv = BRIDGE2_ProcessRxneInterrupt(0xAB);  /* DATA 1 */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	
//	rv = BRIDGE2_ProcessRxneInterrupt(0xCD);  /* DATA 2 */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	
//	rv = BRIDGE2_ProcessRxneInterrupt(0x00);  /* CHECK */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	
//	
//	/* Data block is received, the bridge should answer an ACK to the computer ...  */
//	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CTRL BYTE */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	TEST_ASSERT_EQUAL_UINT8(SM_ACK_BLOCK, byte);
//	
//	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* ACK CHECK */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
//	
//	
//	/* The data block should be processed by the bridge now ...                         */
//	/* The bridge is expected to send the data to the card and get back the answer ...  */
//	rv = BRIDGE2_ProcessTimerInterrupt();
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	
//	
//	
//	/* We check that the bridge sends back to the computer the datas received from the card ...  */
//	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* Control block */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
//	
//	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 1 */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
//	
//	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 2 */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
//	
//	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* LEN 3 */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	TEST_ASSERT_EQUAL_UINT8(0x02, byte);
//	
//	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* DATA 1 */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	TEST_ASSERT_EQUAL_UINT8(0x90, byte);
//	
//	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* DATA 2 */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
//	
//	rv = BRIDGE2_ProcessTxeInterrupt(&byte);  /* CHECK */
//	TEST_ASSERT_TRUE(rv == BRIDGE2_OK);
//	TEST_ASSERT_EQUAL_UINT8(0x00, byte);
//	
//}




//void test_BRIDGE2_TxeInterruptAtWrongTime(void){
//	
//}


// 2 in a row
// check empty return
// checking the interruot enable/disable callbacks

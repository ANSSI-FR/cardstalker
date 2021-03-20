#include "unity.h"

#include "bytes_buffer.h"
#include "tests_buffer.h"




#ifdef TEST




void setUp(void){
	
}


void tearDown(void){
	
}


int main(int argc, char *argv[]){
	UNITY_BEGIN();
	
	RUN_TEST(test_BUFF_Init_shouldSetInitValues);
	RUN_TEST(test_BUFF_Enqueue_shouldWork);
	RUN_TEST(test_BUFF_Dequeue_shouldWork);
	RUN_TEST(test_BUFF_Enqueue_shouldNotIfFull);
	RUN_TEST(test_BUFF_Dequeue_shouldNotIfEmpty);
	RUN_TEST(test_BUFF_Dequeue_shouldWorkIfFull);
	RUN_TEST(test_BUFF_IsFull_shouldWork_Case01);
	RUN_TEST(test_BUFF_IsFull_shouldWork_Case02);
	RUN_TEST(test_BUFF_case01);
	RUN_TEST(test_BUFF_IsEmpty_shouldWork);
	RUN_TEST(test_BUFF_case02);
	
	return UNITY_END();
}
#endif





void test_BUFF_Init_shouldSetInitValues(void){
	BUFF_Buffer buffer;
	BUFF_Status retVal;
	
	
	retVal = BUFF_Init(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	
	TEST_ASSERT_EQUAL_UINT32(0x00000000, buffer.writeIndex);
	TEST_ASSERT_EQUAL_UINT32(0x00000000, buffer.readIndex);
	TEST_ASSERT_EQUAL_UINT32(0x00000000, buffer.currentSize);
}




void test_BUFF_Enqueue_shouldWork(void){
	BUFF_Buffer buffer;
	BUFF_Status retVal;
	uint8_t byte1 = 0xFE;
	uint8_t byte2 = 0xFD;
	
	
	if(BUFF_MAX_SIZE < 2){
		TEST_IGNORE();
	}
	
	retVal = BUFF_Init(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_Enqueue(&buffer, byte1);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);	
	TEST_ASSERT_EQUAL_UINT8(byte1, buffer.array[0]);
	
	retVal = BUFF_Enqueue(&buffer, byte2);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);	
	TEST_ASSERT_EQUAL_UINT8(byte2, buffer.array[1]);
}



void test_BUFF_Dequeue_shouldWork(void){
	BUFF_Buffer buffer;
	BUFF_Status retVal;
	uint8_t byte1 = 0xFE;
	uint8_t byte2 = 0xFD;
	uint8_t byte3, byte4;
	
	
	if(BUFF_MAX_SIZE < 2){
		TEST_IGNORE();
	}
	
	retVal = BUFF_Init(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_Enqueue(&buffer, byte1);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);	
	
	retVal = BUFF_Enqueue(&buffer, byte2);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_Dequeue(&buffer, &byte3);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_Dequeue(&buffer, &byte4);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	
	TEST_ASSERT_EQUAL_UINT8(byte1, byte3);
	TEST_ASSERT_EQUAL_UINT8(byte2, byte4);
}


void test_BUFF_Enqueue_shouldNotIfFull(void){
	BUFF_Buffer buffer;
	BUFF_Status retVal;
	uint32_t i;
	
	
	retVal = BUFF_Init(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	
	for(i=0; i<BUFF_MAX_SIZE; i++){
		retVal = BUFF_Enqueue(&buffer, 0xFF);
		TEST_ASSERT_TRUE(retVal == BUFF_OK);
	}
	
	retVal = BUFF_Enqueue(&buffer, 0xFF);
	TEST_ASSERT_FALSE(retVal == BUFF_OK);	
}


void test_BUFF_Dequeue_shouldNotIfEmpty(void){
	BUFF_Buffer buffer;
	BUFF_Status retVal;
	uint8_t byte;
	
	
	retVal = BUFF_Init(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_Dequeue(&buffer, &byte);
	TEST_ASSERT_FALSE(retVal == BUFF_OK);	
}




void test_BUFF_Dequeue_shouldWorkIfFull(void){
	BUFF_Buffer buffer;
	BUFF_Status retVal;
	uint32_t i;
	uint8_t byte;
	
	
	retVal = BUFF_Init(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	
	for(i=0; i<BUFF_MAX_SIZE; i++){
		retVal = BUFF_Enqueue(&buffer, 0xFF);
		TEST_ASSERT_TRUE(retVal == BUFF_OK);
	}
	
	retVal = BUFF_Dequeue(&buffer, &byte);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8(0xFF, byte);
}


void test_BUFF_IsFull_shouldWork_Case01(void){
	BUFF_Buffer buffer;
	BUFF_Status retVal;
	uint32_t i;
	
	
	retVal = BUFF_Init(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_IsFull(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_NO);


	for(i=0; i<BUFF_MAX_SIZE; i++){
		retVal = BUFF_Enqueue(&buffer, 0xFF);
		TEST_ASSERT_TRUE(retVal == BUFF_OK);
	}
	
	retVal = BUFF_IsFull(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
}


void test_BUFF_IsFull_shouldWork_Case02(void){
	BUFF_Buffer buffer;
	BUFF_Status retVal;
	uint32_t i;
	uint8_t byte;
	
	
	retVal = BUFF_Init(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_Enqueue(&buffer, 0xFF);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_Dequeue(&buffer, &byte);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);


	for(i=0; i<BUFF_MAX_SIZE; i++){
		retVal = BUFF_Enqueue(&buffer, 0xFF);
		TEST_ASSERT_TRUE(retVal == BUFF_OK);
	}
	
	retVal = BUFF_IsFull(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
}




void test_BUFF_case01(void){
	BUFF_Buffer buffer;
	BUFF_Status retVal;
	uint32_t i;
	uint8_t byte;
	
	
	if(BUFF_MAX_SIZE < 2){
		TEST_IGNORE();
	}
	
	
	retVal = BUFF_Init(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);


	for(i=0; i<BUFF_MAX_SIZE-1; i++){
		retVal = BUFF_Enqueue(&buffer, 0xFF);
		TEST_ASSERT_TRUE(retVal == BUFF_OK);
	}
	
	for(i=0; i<BUFF_MAX_SIZE-1; i++){
		retVal = BUFF_Dequeue(&buffer, &byte);
		TEST_ASSERT_TRUE(retVal == BUFF_OK);
	}
	
	for(i=0; i<BUFF_MAX_SIZE; i++){
		retVal = BUFF_Enqueue(&buffer, (uint8_t)(i));
		TEST_ASSERT_TRUE(retVal == BUFF_OK);
	}
	
	
	for(i=0; i<BUFF_MAX_SIZE-1; i++){
		retVal = BUFF_Dequeue(&buffer, &byte);
		TEST_ASSERT_TRUE(retVal == BUFF_OK);
		TEST_ASSERT_EQUAL_UINT8((uint8_t)(i), byte);
	}
}



void test_BUFF_IsEmpty_shouldWork(void){
	BUFF_Buffer buffer;
	BUFF_Status retVal;
	uint32_t i;
	uint8_t byte;
	
	
	retVal = BUFF_Init(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_IsEmpty(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	
	retVal = BUFF_Enqueue(&buffer, 0xFF);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_IsEmpty(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_NO);
	
	retVal = BUFF_Dequeue(&buffer, &byte);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8(0xFF, byte);
	
	retVal = BUFF_IsEmpty(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
}



void test_BUFF_case02(void){
	BUFF_Buffer buffer;
	BUFF_Status retVal;
	uint32_t size;
	uint8_t byte;
	
	
	if(BUFF_MAX_SIZE < 4){
		TEST_IGNORE();
	}
	
	
	retVal = BUFF_Init(&buffer);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);

	
	
	retVal = BUFF_Enqueue(&buffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_Enqueue(&buffer, (uint8_t)'e');
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_Enqueue(&buffer, (uint8_t)'s');
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	retVal = BUFF_Enqueue(&buffer, (uint8_t)'t');
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	
	
	
	retVal = BUFF_GetCurrentSize(&buffer, &size);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT32(4, size);
	
	
	
	retVal = BUFF_Dequeue(&buffer, &byte);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	retVal = BUFF_Dequeue(&buffer, &byte);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'e', byte);
	
	retVal = BUFF_Dequeue(&buffer, &byte);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'s', byte);
	
	retVal = BUFF_Dequeue(&buffer, &byte);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT8((uint8_t)'t', byte);
	
	
	
	retVal = BUFF_GetCurrentSize(&buffer, &size);
	TEST_ASSERT_TRUE(retVal == BUFF_OK);
	TEST_ASSERT_EQUAL_UINT32(0, size);
}

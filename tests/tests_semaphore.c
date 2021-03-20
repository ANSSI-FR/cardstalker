#include "unity.h"

#include "semaphore.h"
#include "tests_semaphore.h"




#ifdef TEST




void setUp(void){
	
}


void tearDown(void){
	
}


int main(int argc, char *argv[]){
	UNITY_BEGIN();
	
	RUN_TEST(test_SEM_Case01);
	RUN_TEST(test_SEM_Case02);
	RUN_TEST(test_SEM_Case03);

	
	return UNITY_END();
}
#endif




void test_SEM_Case01(void){
	SEM_Handle semaphore;
	SEM_Status rv;
	
	
	rv = SEM_Init(&semaphore, 1);
	TEST_ASSERT_TRUE(rv == SEM_OK);	
	
	rv = SEM_Lock(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_OK);	
	
	rv = SEM_IsLocked(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_LOCKED);
	
	rv = SEM_Release(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_OK);		
	
	rv = SEM_IsLocked(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_UNLOCKED);
}



void test_SEM_Case02(void){
	SEM_Handle semaphore;
	SEM_Status rv;
	
	
	rv = SEM_Init(&semaphore, 2);
	TEST_ASSERT_TRUE(rv == SEM_OK);	
	
	rv = SEM_Lock(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_OK);	
	
	rv = SEM_IsLocked(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_UNLOCKED);
	
	rv = SEM_Lock(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_OK);	
	
	rv = SEM_IsLocked(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_LOCKED);
	
	rv = SEM_Release(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_OK);		
	
	rv = SEM_IsLocked(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_UNLOCKED);
}




void test_SEM_Case03(void){
	SEM_Handle semaphore;
	SEM_Status rv;
	
	
	rv = SEM_Init(&semaphore, 1);
	TEST_ASSERT_TRUE(rv == SEM_OK);	
	
	
	rv = SEM_Lock(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_OK);	
	
	rv = SEM_TryLock(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_LOCKED);
	
	rv = SEM_Release(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_OK);		
	
	rv = SEM_TryLock(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_UNLOCKED);
	
	rv = SEM_IsLocked(&semaphore);
	TEST_ASSERT_TRUE(rv == SEM_LOCKED);
}

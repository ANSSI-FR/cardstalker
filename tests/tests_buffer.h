#ifndef __READER_TESTS_BUFFER_H__
#define __READER_TESTS_BUFFER_H__






void setUp(void);
void tearDown(void);
int main(int argc, char *argv[]);


void test_BUFF_Init_shouldSetInitValues(void);
void test_BUFF_Enqueue_shouldWork(void);
void test_BUFF_Dequeue_shouldWork(void);
void test_BUFF_Enqueue_shouldNotIfFull(void);
void test_BUFF_Dequeue_shouldNotIfEmpty(void);
void test_BUFF_Dequeue_shouldWorkIfFull(void);
void test_BUFF_IsFull_shouldWork_Case01(void);
void test_BUFF_IsFull_shouldWork_Case02(void);
void test_BUFF_case01(void);
void test_BUFF_IsEmpty_shouldWork(void);
void test_BUFF_case02(void);





#endif

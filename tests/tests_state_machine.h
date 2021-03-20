#ifndef __TESTS_STATE_MACHINE_H__
#define __TESTS_STATE_MACHINE_H__


#include "state_machine.h"



void setUp(void);
void tearDown(void);
int main();


SM_Status SM_BlockSentCallback(SM_Handle *pHandle);
SM_Status SM_BlockRecievedCallback(SM_Handle *pHandle);
SM_Status SM_EnableRxneInterrupt_Callback(SM_Handle *pHandle);
SM_Status SM_DisableRxneInterrupt_Callback(SM_Handle *pHandle);
SM_Status SM_EnableTxeInterrupt_Callback(SM_Handle *pHandle);
SM_Status SM_DisableTxeInterrupt_Callback(SM_Handle *pHandle);
SM_Status SM_ACK_BLOCK_ReceivedCallback(SM_Handle *pHandle);


void test_SM_ReceiveDataBlockShouldWork(void);
void test_SM_ReceiveEmptyDataBlockShouldWork(void);
void test_SM_ReceiveControlBlockShouldWork(void);
void test_SM_TwoReceiveInARow(void);
void test_SM_SendDataBlockShouldWork(void);
void test_SM_TwoSendInARow(void);
void test_SM_SendEmptyDataBlockShouldWork(void);
void test_SM_SendDataBlockWhenRcvOngoing_case01(void);
void test_SM_SendDataBlockWhenRcvOngoing_case02(void);
void test_SM_SendControlBlockShouldWork(void);
void test_SM_SendDataBlock_anotherBlockInsteadAck(void);
void test_SM_SerializationOfDataBlockSendCalls(void);
void test_SM_SerializationOfNonAckedBlockSendCalls(void);
void test_SM_rcvMutexShouldWork(void);
void test_SM_SendMutexShouldWork(void);
void test_SM_ExtraParasitByteReceived(void);
void test_SM_checkAckIsSentAfterDataRcpt(void);
void test_SM_checkAckIsSentAfterCtrlRcpt(void);
void test_SM_checkAckIsCorrectlyQueued(void);
void test_SM_shouldNotWaitForAckAfterBusyBlock(void);
void test_SM_EvolveStateOnByteTransmission_shouldResetTxeWhenDone(void);
void test_SM_ACK_BLOCK_ReceivedCallback_Case01(void);
void test_SM_ACK_BLOCK_ReceivedCallback_Case02(void);
void test_SM_ACK_BLOCK_ReceivedCallback_Case03(void);




#endif

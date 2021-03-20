#ifndef __TESTS_BRIDGE_ADVANCED_H__
#define __TESTS_BRIDGE_ADVANCED_H__



#include "bridge_advanced.h"



void setUp(void);
void tearDown(void);
int main(void);


BRIDGE2_Status BRIDGE2_EnableTxeInterrupt_Callback(void);
BRIDGE2_Status BRIDGE2_DisableTxeInterrupt_Callback(void);
BRIDGE2_Status BRIDGE2_EnableRxneInterrupt_Callback(void);
BRIDGE2_Status BRIDGE2_DisableRxneInterrupt_Callback(void);


void test_BRIDGE2_dataBlockShouldWork_Case01(void);
void test_BRIDGE2_dataBlockShouldWork_Case02(void);
void test_BRIDGE2_dataBlockNoAnswerFromCard(void);
void test_BRIDGE2_coldReset(void);
void test_BRIDGE2_TwoProcessesInARow_Case01(void);





#endif

#ifndef TEST


#include "main.h"
#include "stm32f4xx_hal.h"
#include "bridge_advanced.h"
#include "stm32f4xx_hal_uart_custom.h"




UART_HandleTypeDef uartHandleStruct;
TIM_HandleTypeDef timerHandleStruct;
uint8_t globalBuff;


void initUartHandle(UART_HandleTypeDef *uartHandleStruct);
void initUartHardware(void);

void initTimerHandle(TIM_HandleTypeDef *pTimerHandle);
void initTimerHardware(void);

void ErrorHandler(void);



int main(void){
	READER_HAL_CommSettings settings;
	BRIDGE2_Status rv;
	READER_Status readerRv;
	HAL_StatusTypeDef halRv;
	
	
	/* Initializing the reader lib ...  */
	readerRv = READER_HAL_InitWithDefaults(&settings);
	if(readerRv != READER_OK) ErrorHandler();
	
	HAL_NVIC_SetPriority(SysTick_IRQn, 0x00, 0U);
	/* Initializing the computer-bridge communication ...  */
	initUartHandle(&uartHandleStruct);
	initUartHardware();
	
	halRv = HAL_UART_Init(&uartHandleStruct);
	if(halRv != HAL_OK) ErrorHandler();
	
	/* Initiating timer ...  */
	initTimerHandle(&timerHandleStruct);
	initTimerHardware();
	
	halRv = HAL_TIM_Base_Init(&timerHandleStruct);
	if(halRv != HAL_OK) ErrorHandler();
	
	
	/* Initializing the bridge state machine and running it ...  */
	rv = BRIDGE2_Init(&settings);
	if(rv != BRIDGE2_OK) ErrorHandler();
	
	rv = BRIDGE2_Run();
	if(rv != BRIDGE2_OK) ErrorHandler();
	
	
	while(1);
	
	
	return 0;
}



BRIDGE2_Status BRIDGE2_EnableTxeInterrupt_Callback(void){
	HAL_UART_Transmit_IT_continuous(&uartHandleStruct, &globalBuff);
	SET_BIT(uartHandleStruct.Instance->CR1, USART_CR1_TXEIE);
	
	
	return BRIDGE2_OK;
}


BRIDGE2_Status BRIDGE2_DisableTxeInterrupt_Callback(void){
	CLEAR_BIT(uartHandleStruct.Instance->CR1, USART_CR1_TXEIE);
	
	return BRIDGE2_OK;
}


BRIDGE2_Status BRIDGE2_EnableRxneInterrupt_Callback(void){
	HAL_UART_Receive_IT_continuous(&uartHandleStruct);
		
	return BRIDGE2_OK;
}


BRIDGE2_Status BRIDGE2_DisableRxneInterrupt_Callback(void){
	HAL_UART_Abort_IT(&uartHandleStruct);
	
	return BRIDGE2_OK;
}


BRIDGE2_Status BRIDGE2_EnableTimerInterrupt_Callback(void){
	HAL_StatusTypeDef halRv;
	
	
	halRv = HAL_TIM_Base_Start_IT(&timerHandleStruct);
	if(halRv != HAL_OK) ErrorHandler();
	
	return BRIDGE2_OK;
}

BRIDGE2_Status BRIDGE2_DisableTimerInterrupt_Callback(void){
	HAL_StatusTypeDef halRv;
	
	
	halRv = HAL_TIM_Base_Stop_IT(&timerHandleStruct);
	if(halRv != HAL_OK) ErrorHandler();
	
	return BRIDGE2_OK;
}


void HAL_UART_RxCpltCallback_continuous(UART_HandleTypeDef *huart, uint16_t data){
	BRIDGE2_Status rv;
	
	rv = BRIDGE2_ProcessRxneInterrupt((uint8_t)(data));
	if(rv != BRIDGE2_OK) ErrorHandler();
}



void HAL_UART_Transmit_IT_PrechargeCallback(UART_HandleTypeDef *huart, uint8_t *byteToSend, uint8_t *dataAvailable){
	BRIDGE2_Status rv;
	uint8_t byte;
	
	
	rv = BRIDGE2_ProcessTxeInterrupt(&byte);
	if((rv != BRIDGE2_OK) && (rv != BRIDGE2_EMPTY)) ErrorHandler();
	
	if(rv == BRIDGE2_OK){
		*byteToSend = byte;
		*dataAvailable = 1;
	}
	
	if(rv == BRIDGE2_EMPTY){
		*dataAvailable = 0;
	}
}


void initUartHandle(UART_HandleTypeDef *uartHandleStruct){
	uartHandleStruct->Instance = USART1;
	uartHandleStruct->Init.BaudRate = BRIDGE2_DEFAULT_COMPUTER_BAUDRATE;
	uartHandleStruct->Init.Mode = UART_MODE_TX_RX;
	uartHandleStruct->Init.OverSampling = UART_OVERSAMPLING_16;
	uartHandleStruct->Init.Parity = UART_PARITY_NONE;
	uartHandleStruct->Init.StopBits = UART_STOPBITS_1;
	uartHandleStruct->Init.WordLength = UART_WORDLENGTH_8B;
	uartHandleStruct->Init.HwFlowCtl = UART_HWCONTROL_NONE;
}


void initUartHardware(void){
	GPIO_InitTypeDef gpioInitStruct;
	
	/* Initialisation pin TX */
	gpioInitStruct.Pin = GPIO_PIN_6;
	gpioInitStruct.Mode = GPIO_MODE_AF_PP;
	gpioInitStruct.Pull = GPIO_PULLUP;
	gpioInitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	gpioInitStruct.Alternate = GPIO_AF7_USART1;
	
	__HAL_RCC_GPIOB_CLK_ENABLE();
	HAL_GPIO_Init(GPIOB, &gpioInitStruct);
	
	
	/* Initialisation pin RX */
	gpioInitStruct.Pin = GPIO_PIN_10;
	gpioInitStruct.Mode = GPIO_MODE_AF_PP;
	gpioInitStruct.Pull = GPIO_PULLUP;
	gpioInitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	gpioInitStruct.Alternate = GPIO_AF7_USART1;
	
	__HAL_RCC_GPIOA_CLK_ENABLE();
	HAL_GPIO_Init(GPIOA, &gpioInitStruct);
	
	/* Activation clock bloc UART */
	__HAL_RCC_USART1_CLK_ENABLE();
	
	
	/* Configuration des Interruptions */
	HAL_NVIC_SetPriority(USART1_IRQn, 0x0E, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);
	
}


void initTimerHardware(void){
	__HAL_RCC_TIM5_CLK_ENABLE();
	
	HAL_NVIC_SetPriority(TIM5_IRQn, 0x0F, 0);
	NVIC_EnableIRQ(TIM5_IRQn);
}


void initTimerHandle(TIM_HandleTypeDef *pTimerHandle){
	pTimerHandle->Instance = TIM5;
	pTimerHandle->Init.Prescaler = 10000;
	pTimerHandle->Init.CounterMode = TIM_COUNTERMODE_UP;
	pTimerHandle->Init.Period = 500;
	pTimerHandle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
}


void TIM5_IRQHandler(void) {
	BRIDGE2_Status rv;
	
	if (__HAL_TIM_GET_FLAG(&timerHandleStruct, TIM_FLAG_UPDATE) == SET) {
		__HAL_TIM_CLEAR_FLAG(&timerHandleStruct, TIM_FLAG_UPDATE);
		
		rv = BRIDGE2_ProcessTimerInterrupt();
		if(rv != BRIDGE2_OK) ErrorHandler();
	}
}


void ErrorHandler(void){
	while(1){
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
		HAL_Delay(100);
	}
}

#endif

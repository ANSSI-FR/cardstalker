/**
 * \file bridge.c
 * \copyright This file is part of the Card-Stalker project and is distributed under the GPLv3 license. See LICENSE file in the root directory of the project.
 * This file provides a top-level API for enabling the user to initialize and run the computer-card bridge.
 * The content of this file is now deprecated and should not be used. this file implements the first design of teh bridge previously implemented as a simple byte repeater.
 * See bridge_advanced.c for the current version.
 */


#ifndef BRIDGE2

#include "bridge.h"
#include "bytes_buffer.h"
#include "reader_lib.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart_custom.h"

#include "main.h"



/* Static functions declarations ... */
static BRIDGE_Status BRIDGE_InitUartHandle(UART_HandleTypeDef *uartHandleStruct);
static BRIDGE_Status BRIDGE_InitHardware(void);



/* Global variables declarations ...  */
BUFF_Buffer globalSendBuffer;
BUFF_Buffer globalReceiveBuffer;
READER_HAL_CommSettings globalReaderHalSettings;	

uint8_t globalBuff;
UART_HandleTypeDef uartHandleStruct;



/**
 * \fn BRIDGE_Status BRIDGE_InitWithDefaults(void)
 * This functions initialize the hardware, the FIFOs and the interruptions in order to run next the BRIDGE_Run() function.
 * Please note that only one bridge instance is supported. If you run twice this function you will re-initialize the current bridge.
 * \return This function returns a BRIDGE_Status execution code.
 */
BRIDGE_Status BRIDGE_InitWithDefaults(void){
	BRIDGE_Status retVal;
	READER_Status retVal2;
	BUFF_Status retVal4;
	HAL_StatusTypeDef retVal3;
	
	
	
	
	/* Initializing the FIFOs ...  */
	retVal4 = BUFF_Init(&globalSendBuffer);
	if(retVal4 != BUFF_OK) return BRIDGE_ERR;
	
	retVal4 = BUFF_Init(&globalReceiveBuffer);
	if(retVal4 != BUFF_OK) return BRIDGE_ERR;
	
	
	/* Initializing the hardware for the bridge-card communication (essentially setting up the reader hal library) ...  */
	/* Warning : the order is important ...  READER_HAL_InitWithDefaults() function also initiates the ST HAL which is also used by the subsequent functions ... */
	retVal2 = READER_HAL_InitWithDefaults(&globalReaderHalSettings);
	if(retVal2 != READER_OK) return BRIDGE_ERR;
	
	retVal2 = READER_HAL_DoColdReset();
	if(retVal2 != READER_OK) return BRIDGE_ERR;
	
	
	/* Initializing the hardware for the computer-bridge communication (essentially pins and interruptions) ...  */
	retVal = BRIDGE_InitHardware();
	if(retVal != BRIDGE_OK) return BRIDGE_ERR;
	
	retVal = BRIDGE_InitUartHandle(&uartHandleStruct);
	if(retVal != BRIDGE_OK) return BRIDGE_ERR;
	
	retVal3 = HAL_UART_Init(&uartHandleStruct);
	if(retVal3 != HAL_OK) return BRIDGE_ERR;
	
	HAL_UART_Transmit_IT_continuous(&uartHandleStruct, &globalBuff);
	HAL_UART_Receive_IT_continuous(&uartHandleStruct);
	
	
	return BRIDGE_OK;	
}


/**
 * \fn BRIDGE_Status BRIDGE_Run(void)
 * This function runs the bridge. This function is in blocking mode, the bridge is working while this function is executing.
 * \return On error bridge stops running and returns a BRIDGE_Status execution code.
 */
BRIDGE_Status BRIDGE_Run(void){
	BUFF_Status empty, retVal;
	READER_Status rv;
	HAL_StatusTypeDef hal;
	uint8_t byte;
	
	
	while(1){
		empty = BUFF_IsEmpty(&globalSendBuffer);
		if((empty != BUFF_OK) && (empty != BUFF_NO)) return BRIDGE_ERR;
		
		if(empty == BUFF_NO){
			retVal = BUFF_Dequeue(&globalSendBuffer, &byte);
			if(retVal != BUFF_OK) return BRIDGE_ERR;
			
			rv = READER_HAL_SendChar(&globalReaderHalSettings, READER_HAL_PROTOCOL_T1, byte, BRIDGE_DEFAULT_RECEIVE_SEND_TIMEOUT);
			if(rv != READER_OK) return BRIDGE_ERR;
		}
		else{
			while(!(USART2->SR & USART_SR_TC));
			rv = READER_HAL_RcvChar(&globalReaderHalSettings, READER_HAL_PROTOCOL_T1, &byte, BRIDGE_DEFAULT_RECEIVE_SEND_TIMEOUT);
			if(rv == READER_OK){
				BUFF_Enqueue(&globalReceiveBuffer, byte);
				
				/* Enable the UART Transmit data register empty Interrupt */
				SET_BIT(uartHandleStruct.Instance->CR1, USART_CR1_TXEIE);
		
			}
			else if(rv != READER_TIMEOUT){
				return BRIDGE_ERR;
			}
			else{
				//return BRIDGE_ERR;
			}
		}
	}
	
	return BRIDGE_ERR;
}


void HAL_UART_RxCpltCallback_continuous(UART_HandleTypeDef *uartHandleStruct, uint16_t data){
	BUFF_Enqueue(&globalSendBuffer, (uint8_t)(data));
}


void HAL_UART_Transmit_IT_PrechargeCallback(UART_HandleTypeDef *huart, uint8_t *byteToSend, uint8_t *dataAvailable){
	BUFF_Status rv;
	uint8_t byte;
	
	rv = BUFF_Dequeue(&globalReceiveBuffer, &byte);
	if((rv != BUFF_OK) && (rv != BUFF_EMPTY)){
		//ErrorHandler();
	}
	
	
	if(rv == BUFF_OK){
		*byteToSend = byte;
		*dataAvailable = 1;
	}
	
	if(rv == BUFF_EMPTY){
		*dataAvailable = 0;
	}
	
}


static BRIDGE_Status BRIDGE_InitUartHandle(UART_HandleTypeDef *uartHandleStruct){
	uartHandleStruct->Instance = USART1;
	uartHandleStruct->Init.BaudRate = BRIDGE_DEFAULT_COMPUTER_BAUDRATE;
	uartHandleStruct->Init.Mode = UART_MODE_TX_RX;
	uartHandleStruct->Init.OverSampling = UART_OVERSAMPLING_16;
	uartHandleStruct->Init.Parity = UART_PARITY_NONE;
	uartHandleStruct->Init.StopBits = UART_STOPBITS_1;
	uartHandleStruct->Init.WordLength = UART_WORDLENGTH_8B;
	uartHandleStruct->Init.HwFlowCtl = UART_HWCONTROL_NONE;
	
	
	return BRIDGE_OK;
}


static BRIDGE_Status BRIDGE_InitHardware(void){
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
	HAL_NVIC_SetPriority(USART1_IRQn, 0, 1);
	HAL_NVIC_EnableIRQ(USART1_IRQn);
	
		
	return BRIDGE_OK;
}


#endif

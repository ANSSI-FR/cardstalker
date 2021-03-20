#include "stm32f4xx_hal_uart.h"



void HAL_UART_IRQHandler_continuous(UART_HandleTypeDef *huart);
void HAL_UART_RxCpltCallback_continuous(UART_HandleTypeDef *huart, uint16_t data);
HAL_StatusTypeDef HAL_UART_Receive_IT_continuous(UART_HandleTypeDef *huart);
void HAL_UART_Transmit_IT_PrechargeCallback(UART_HandleTypeDef *huart, uint8_t *byteToSend, uint8_t *dataAvailable);
HAL_StatusTypeDef HAL_UART_Transmit_IT_continuous(UART_HandleTypeDef *huart, uint8_t *pData);

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_uart_custom.h"





/* Private function prototypes -----------------------------------------------*/
/** @addtogroup UART_Private_Functions   UART Private Functions
  * @{
  */
static void UART_DMAAbortOnError(DMA_HandleTypeDef *hdma);
static HAL_StatusTypeDef UART_Receive_IT_continuous(UART_HandleTypeDef *huart);
static void UART_EndRxTransfer(UART_HandleTypeDef *huart);
static HAL_StatusTypeDef UART_EndTransmit_IT(UART_HandleTypeDef *huart);
static HAL_StatusTypeDef UART_Transmit_IT(UART_HandleTypeDef *huart);
static HAL_StatusTypeDef UART_Transmit_IT_continuous(UART_HandleTypeDef *huart);







/**
  * @brief  Receives an amount of data in non blocking mode 
  * @param  huart pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @param  pData Pointer to data buffer
  * @param  Size Amount of data to be received
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_Receive_IT_continuous(UART_HandleTypeDef *huart)
{
  /* Check that a Rx process is not already ongoing */ 
  if(huart->RxState == HAL_UART_STATE_READY)
  {
    
    /* Process Locked */
    __HAL_LOCK(huart);
    
    huart->pRxBuffPtr = NULL;
    huart->RxXferSize = 0;
    huart->RxXferCount = 0;
    
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->RxState = HAL_UART_STATE_BUSY_RX;
    
    /* Process Unlocked */
    __HAL_UNLOCK(huart);
        
    /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
    SET_BIT(huart->Instance->CR3, USART_CR3_EIE);

    /* Enable the UART Parity Error and Data Register not empty Interrupts */
    SET_BIT(huart->Instance->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);
    
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY; 
  }
}







/**
  * @brief  This function handles UART interrupt request.
  * @param  huart pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
void HAL_UART_IRQHandler_continuous(UART_HandleTypeDef *huart)
{
   uint32_t isrflags   = READ_REG(huart->Instance->SR);
   uint32_t cr1its     = READ_REG(huart->Instance->CR1);
   uint32_t cr3its     = READ_REG(huart->Instance->CR3);
   uint32_t errorflags = 0x00U;
   uint32_t dmarequest = 0x00U;

  /* If no error occurs */
  errorflags = (isrflags & (uint32_t)(USART_SR_PE | USART_SR_FE | USART_SR_ORE | USART_SR_NE));
  if(errorflags == RESET)
  {
    /* UART in mode Receiver -------------------------------------------------*/
    if(((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
    {
      UART_Receive_IT_continuous(huart);
      return;
    }
  }  

  /* If some errors occur */
  if((errorflags != RESET) && (((cr3its & USART_CR3_EIE) != RESET) || ((cr1its & (USART_CR1_RXNEIE | USART_CR1_PEIE)) != RESET)))
  {
    /* UART parity error interrupt occurred ----------------------------------*/
    if(((isrflags & USART_SR_PE) != RESET) && ((cr1its & USART_CR1_PEIE) != RESET))
    {
      huart->ErrorCode |= HAL_UART_ERROR_PE;
    }
    
    /* UART noise error interrupt occurred -----------------------------------*/
    if(((isrflags & USART_SR_NE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
    {
      huart->ErrorCode |= HAL_UART_ERROR_NE;
    }
    
    /* UART frame error interrupt occurred -----------------------------------*/
    if(((isrflags & USART_SR_FE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
    {
      huart->ErrorCode |= HAL_UART_ERROR_FE;
    }
    
    /* UART Over-Run interrupt occurred --------------------------------------*/
    if(((isrflags & USART_SR_ORE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
    { 
      huart->ErrorCode |= HAL_UART_ERROR_ORE;
    }

    /* Call UART Error Call back function if need be --------------------------*/    
    if(huart->ErrorCode != HAL_UART_ERROR_NONE)
    {
      /* UART in mode Receiver -----------------------------------------------*/
      if(((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
      {
        UART_Receive_IT_continuous(huart);
      }

      /* If Overrun error occurs, or if any error occurs in DMA mode reception,
         consider error as blocking */
      dmarequest = HAL_IS_BIT_SET(huart->Instance->CR3, USART_CR3_DMAR);
      if(((huart->ErrorCode & HAL_UART_ERROR_ORE) != RESET) || dmarequest)
      {
        /* Blocking error : transfer is aborted
           Set the UART state ready to be able to start again the process,
           Disable Rx Interrupts, and disable Rx DMA request, if ongoing */
        UART_EndRxTransfer(huart);
        
        /* Disable the UART DMA Rx request if enabled */
        if(HAL_IS_BIT_SET(huart->Instance->CR3, USART_CR3_DMAR))
        {
          CLEAR_BIT(huart->Instance->CR3, USART_CR3_DMAR);
          
          /* Abort the UART DMA Rx channel */
          if(huart->hdmarx != NULL)
          {
            /* Set the UART DMA Abort callback : 
               will lead to call HAL_UART_ErrorCallback() at end of DMA abort procedure */
            huart->hdmarx->XferAbortCallback = UART_DMAAbortOnError;
            if(HAL_DMA_Abort_IT(huart->hdmarx) != HAL_OK)
            {
              /* Call Directly XferAbortCallback function in case of error */
              huart->hdmarx->XferAbortCallback(huart->hdmarx);
            }
          }
          else
          {
            /* Call user error callback */
            HAL_UART_ErrorCallback(huart);
          }
        }
        else
        {
          /* Call user error callback */
          HAL_UART_ErrorCallback(huart);
        }
      }
      else
      {
        /* Non Blocking error : transfer could go on. 
           Error is notified to user through user error callback */
        HAL_UART_ErrorCallback(huart);
        huart->ErrorCode = HAL_UART_ERROR_NONE;
      }
    }
    return;
  } /* End if some error occurs */

  /* UART in mode Transmitter ------------------------------------------------*/
  if(((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
  {
    UART_Transmit_IT_continuous(huart);
    return;
  }
  
  /* UART in mode Transmitter end --------------------------------------------*/
  if(((isrflags & USART_SR_TC) != RESET) && ((cr1its & USART_CR1_TCIE) != RESET))
  {
    UART_EndTransmit_IT(huart);
    return;
  }
}




/**
  * @brief  Receives an amount of data in non blocking mode 
  * @param  huart pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
static HAL_StatusTypeDef UART_Receive_IT_continuous(UART_HandleTypeDef *huart)
{
  uint16_t data;
  
  /* Check that a Rx process is ongoing */
  if(huart->RxState == HAL_UART_STATE_BUSY_RX) 
  {
    if(huart->Init.WordLength == UART_WORDLENGTH_9B)
    {
      if(huart->Init.Parity == UART_PARITY_NONE)
      {
        data = (uint16_t)(huart->Instance->DR & (uint16_t)0x01FF);
        //huart->pRxBuffPtr += 2U;
      }
      else
      {
        data = (uint16_t)(huart->Instance->DR & (uint16_t)0x00FF);
        //huart->pRxBuffPtr += 1U;
      }
    }
    else
    {
      if(huart->Init.Parity == UART_PARITY_NONE)
      {
        data = (uint8_t)(huart->Instance->DR & (uint8_t)0x00FF);
      }
      else
      {
        data = (uint8_t)(huart->Instance->DR & (uint8_t)0x007F);
      }
    }


    /* Disable the UART Parity Error Interrupt and RXNE interrupt*/
    //CLEAR_BIT(huart->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));

    /* Disable the UART Error Interrupt: (Frame error, noise error, overrun error) */
    //CLEAR_BIT(huart->Instance->CR3, USART_CR3_EIE);

    /* Rx process is completed, restore huart->RxState to Ready */
    //huart->RxState = HAL_UART_STATE_READY;
    
    HAL_UART_RxCpltCallback_continuous(huart, data);


    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}




/**
  * @brief  DMA UART communication abort callback, when initiated by HAL services on Error
  *         (To be called at end of DMA Abort procedure following error occurrence).
  * @param  hdma DMA handle.
  * @retval None
  */
static void UART_DMAAbortOnError(DMA_HandleTypeDef *hdma)
{
  UART_HandleTypeDef* huart = ( UART_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
  huart->RxXferCount = 0U;
  huart->TxXferCount = 0U;

  HAL_UART_ErrorCallback(huart);
}





/**
  * @brief  Sends an amount of data in non blocking mode.
  * @param  huart Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
static HAL_StatusTypeDef UART_Transmit_IT(UART_HandleTypeDef *huart)
{
  uint16_t* tmp;
  
  /* Check that a Tx process is ongoing */
  if(huart->gState == HAL_UART_STATE_BUSY_TX)
  {
	  
    if(huart->Init.WordLength == UART_WORDLENGTH_9B)
    {
      tmp = (uint16_t*) huart->pTxBuffPtr;
      huart->Instance->DR = (uint16_t)(*tmp & (uint16_t)0x01FF);
      if(huart->Init.Parity == UART_PARITY_NONE)
      {
        huart->pTxBuffPtr += 2U;
      }
      else
      {
        huart->pTxBuffPtr += 1U;
      }
    } 
    else
    {
      huart->Instance->DR = (uint8_t)(*huart->pTxBuffPtr++ & (uint8_t)0x00FF);
    }

    if(--huart->TxXferCount == 0U)
    {
      /* Disable the UART Transmit Complete Interrupt */
      CLEAR_BIT(huart->Instance->CR1, USART_CR1_TXEIE);

      /* Enable the UART Transmit Complete Interrupt */    
      SET_BIT(huart->Instance->CR1, USART_CR1_TCIE);
    }
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}



/**
  * @brief  Sends an amount of data in non blocking mode.
  * @param  huart Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
static HAL_StatusTypeDef UART_Transmit_IT_continuous(UART_HandleTypeDef *huart)
{
  uint16_t* tmp;
  uint8_t dataAvail;
  uint8_t byteToSend;
  
  /* Check that a Tx process is ongoing */
  if(huart->gState == HAL_UART_STATE_BUSY_TX)
  {
	  
	HAL_UART_Transmit_IT_PrechargeCallback(huart, &byteToSend, &dataAvail);
	
	if(!dataAvail){
		/* Disable the UART Transmit Complete Interrupt */
		CLEAR_BIT(huart->Instance->CR1, USART_CR1_TXEIE);
		
    
		return HAL_OK;
	}
	
	  
    if(huart->Init.WordLength == UART_WORDLENGTH_9B)
    {
      tmp = (uint16_t*)(&byteToSend);
      huart->Instance->DR = (uint16_t)(*tmp & (uint16_t)0x01FF);
      if(huart->Init.Parity == UART_PARITY_NONE)
      {
        //huart->pTxBuffPtr += 2U;
      }
      else
      {
        //huart->pTxBuffPtr += 1U;
      }
    } 
    else
    {
      //huart->Instance->DR = (uint8_t)(*huart->pTxBuffPtr++ & (uint8_t)0x00FF);
      huart->Instance->DR = (uint8_t)(byteToSend & (uint8_t)0x00FF);
    }

    //if(--huart->TxXferCount == 0U)
    //{
    //  /* Disable the UART Transmit Complete Interrupt */
    //  CLEAR_BIT(huart->Instance->CR1, USART_CR1_TXEIE);
	//
    //  /* Enable the UART Transmit Complete Interrupt */    
    //  SET_BIT(huart->Instance->CR1, USART_CR1_TCIE);
    //}
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;
  }
}




/**
  * @brief  This function is called on TX Empty (TXE) interrupt right before the next byte is pushed into the USART data register (DR). This function is aimed to be redifined by the user in order to set the next byte of information to be sent (huart->pTxBuffPtr).
  * @param  huart UART handle.
  * @retval None
  */
__weak void HAL_UART_Transmit_IT_PrechargeCallback(UART_HandleTypeDef *huart, uint8_t *byteToSend, uint8_t *dataAvailable)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART_TxCpltCallback could be implemented in the user file
   */
}




/**
  * @brief  End ongoing Rx transfer on UART peripheral (following error detection or Reception completion).
  * @param  huart UART handle.
  * @retval None
  */
static void UART_EndRxTransfer(UART_HandleTypeDef *huart)
{
  /* Disable RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
  CLEAR_BIT(huart->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
  CLEAR_BIT(huart->Instance->CR3, USART_CR3_EIE);

  /* At end of Rx process, restore huart->RxState to Ready */
  huart->RxState = HAL_UART_STATE_READY;
}



/**
  * @brief  Wraps up transmission in non blocking mode.
  * @param  huart pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
static HAL_StatusTypeDef UART_EndTransmit_IT(UART_HandleTypeDef *huart)
{
  /* Disable the UART Transmit Complete Interrupt */    
  CLEAR_BIT(huart->Instance->CR1, USART_CR1_TCIE);
  
  /* Tx process is ended, restore huart->gState to Ready */
  huart->gState = HAL_UART_STATE_READY;
    
  HAL_UART_TxCpltCallback(huart);
  
  return HAL_OK;
}



/**
  * @brief  Rx Transfer completed callbacks.
  * @param  huart pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
__weak void HAL_UART_RxCpltCallback_continuous(UART_HandleTypeDef *huart, uint16_t data)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART_TxCpltCallback could be implemented in the user file
   */
}





/**
  * @brief  Sends an amount of data in non blocking mode.
  * @param  huart pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @param  pData Pointer to data buffer
  * @param  Size Amount of data to be sent
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_UART_Transmit_IT_continuous(UART_HandleTypeDef *huart, uint8_t *pData)
{
  /* Check that a Tx process is not already ongoing */
  if(huart->gState == HAL_UART_STATE_READY)
  {
    //if((pData == NULL ) || (Size == 0)) 
    if((pData == NULL )) 
    {
      return HAL_ERROR;
    }
    
    /* Process Locked */
    __HAL_LOCK(huart);
    
    huart->pTxBuffPtr = pData;
    huart->TxXferSize = 1;
    huart->TxXferCount = 1;

    huart->ErrorCode = HAL_UART_ERROR_NONE;
    huart->gState = HAL_UART_STATE_BUSY_TX;

    /* Process Unlocked */
    __HAL_UNLOCK(huart);

    /* Enable the UART Transmit data register empty Interrupt */
    SET_BIT(huart->Instance->CR1, USART_CR1_TXEIE);
    
    return HAL_OK;
  }
  else
  {
    return HAL_BUSY;   
  }
}

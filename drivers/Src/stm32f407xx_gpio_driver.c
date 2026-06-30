#include "stm32f407xx_gpio_driver.h"

/*
 * GPIO Pin Interrupt Configuration
 * 1. Pin must be in input configuration.
 * 2. Configure the edge trigger (RT,FT,RFT)
 * 3. Enable interrupt delivery from peripheral to the processor (on peripheral side)
 * 4. Identify the IRQ number on which the processor accepts the interrupt from that pin.
 * 5. Configure the IRQ priority for the identifier IRQ number(processor side)
 * 6. Enable interrupt reception on that IRQ number(processor side)
 * 7. Implement IRQ handler

 * GPIO port is decided by SYSCFG_EXTICR register configuration. (mcu peripheral side)
 * EXTI block does Edge detection (FT,RT), Enable/Disable of interrupt delivery to processor. (mcu peripheral side)
 * Enable/Disable of IRQs are configured in NVIC registers. (processor side)
 */


/*
 * Peripheral clock setup
 */
/********************************************
 * @fn				- GPIO_PeriClockControl
 *
 * @brief			- This function enables or disables peripheral clock for the given GPIO port.
 *
 * @param[in]		- base address of the GPIO peripheral
 * @param[in]		- ENABLE or DISABLE macros
 *
 * @return			- none
 *
 * @Note			- none
 *
 */
void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		if(pGPIOx == GPIOA) GPIOA_PCLK_EN();
		else if(pGPIOx == GPIOB) GPIOB_PCLK_EN();
		else if(pGPIOx == GPIOC) GPIOC_PCLK_EN();
		else if(pGPIOx == GPIOD) GPIOD_PCLK_EN();
		else if(pGPIOx == GPIOE) GPIOE_PCLK_EN();
		else if(pGPIOx == GPIOF) GPIOF_PCLK_EN();
		else if(pGPIOx == GPIOG) GPIOG_PCLK_EN();
		else if(pGPIOx == GPIOH) GPIOH_PCLK_EN();
		else if(pGPIOx == GPIOI) GPIOI_PCLK_EN();
	} else
	{
		if(pGPIOx == GPIOA) GPIOA_PCLK_EN();
		else if(pGPIOx == GPIOB) GPIOB_PCLK_DI();
		else if(pGPIOx == GPIOC) GPIOC_PCLK_DI();
		else if(pGPIOx == GPIOD) GPIOD_PCLK_DI();
		else if(pGPIOx == GPIOE) GPIOE_PCLK_DI();
		else if(pGPIOx == GPIOF) GPIOF_PCLK_DI();
		else if(pGPIOx == GPIOG) GPIOG_PCLK_DI();
		else if(pGPIOx == GPIOH) GPIOH_PCLK_DI();
		else if(pGPIOx == GPIOI) GPIOI_PCLK_DI();
	}
}


/*
 * Init and De-init
 */
void GPIO_Init(GPIO_Handle_t *pGPIOHandle)
{
	uint32_t temp = 0;			// temp register

	// enable the peripheral clock
	GPIO_PeriClockControl(pGPIOHandle->pGPIOx, ENABLE);

	// 1) Configure the mode of gpio pin
	if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode <= GPIO_MODE_ANALOG)
	{
		// non interrupt mode
		temp = ( pGPIOHandle->GPIO_PinConfig.GPIO_PinMode << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber) );
		pGPIOHandle->pGPIOx->MODER &= ~( 0x3 << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber) );		// clearing the bits
		pGPIOHandle->pGPIOx->MODER |= temp; 	// setting the bits

	} else
	{
		// interrupt modes
		if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_FT)
		{
			// 1. configure the FTSR (Falling Trigger Selection Register)
			EXTI->FTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
			// clear the corresponding RTST bit
			EXTI->RTSR &= ~(1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
		}else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RT)
		{
			// 1. configure the RTSR (Rising Trigger Selection Register)
			EXTI->RTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
			// clear the corresponding FTSR bit
			EXTI->FTSR &= ~(1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
		}else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RFT)
		{
			// 1. configure both FTSR and RTSR
			EXTI->RTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
			EXTI->FTSR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
		}

		// 2. configure the GPIO port selection in SYSCFG_EXTICR
		uint8_t temp1 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber / 4; 	// Determine the EXTICR register index (EXTICR1 to EXTICR4)
		uint8_t temp2 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 4;		// Determine the bit-field position within the selected EXTICR register
		uint8_t portcode = GPIO_BASEADDR_TO_CODE(pGPIOHandle->pGPIOx);      // Obtain the port code corresponding to the GPIO port (GPIOA, GPIOB, etc.)
		SYSCFG_PCLK_EN();
		SYSCFG->EXTICR[temp1] = portcode << (temp2 * 4);


		// 3. enable the exti interrupt delivery using IMR
		EXTI->IMR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);

	}

	// 2) Configure the speed of gpio pin
	temp = ( pGPIOHandle->GPIO_PinConfig.GPIO_PinSpeed << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber) );
	pGPIOHandle->pGPIOx->OSPEEDR &= ~( 0x3 << (2 * pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber) );	// clearing the bits
	pGPIOHandle->pGPIOx->OSPEEDR |= temp;		// setting the bits

	// 3) Configure the pupd settings
	temp = ( pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl << (2*pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber) );
	pGPIOHandle->pGPIOx->PUPDR &= ~( 0x3 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber );	// clearing
	pGPIOHandle->pGPIOx->PUPDR |= temp; 	// setting

	// 4) Configure the optype
	temp = ( pGPIOHandle->GPIO_PinConfig.GPIO_PinOPType << (2*pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber) );
	pGPIOHandle->pGPIOx->OTYPER &= ~(0x1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
	pGPIOHandle->pGPIOx->OTYPER |= temp;

	// 5) Configure the alt functionality
	if(pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_ALTFN)
	{
		uint8_t temp1, temp2;									    // Variables to store AFR register index and bit position
		temp1 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber / 8;		// Select AFR[0] for pins 0–7 or AFR[1] for pins 8–15
		temp2 = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber % 8;		// Get position of the pin within AFR register (0–7)

		pGPIOHandle->pGPIOx->AFR[temp1] &= ~(0xF << (4 * temp2));	// Clear the 4-bit field corresponding to the pin

		// Set the alternate function value in the 4-bit field
		pGPIOHandle->pGPIOx->AFR[temp1] |= ( pGPIOHandle->GPIO_PinConfig.GPIO_PinAltFunMode << (4 * temp2) );
	}

}

void GPIO_DeInit(GPIO_RegDef_t *pGPIOx)
{
	if(pGPIOx == GPIOA)	GPIOA_REG_RESET();
	else if(pGPIOx == GPIOB) GPIOB_REG_RESET();
	else if(pGPIOx == GPIOC) GPIOC_REG_RESET();
	else if(pGPIOx == GPIOD) GPIOD_REG_RESET();
	else if(pGPIOx == GPIOE) GPIOE_REG_RESET();
	else if(pGPIOx == GPIOF) GPIOF_REG_RESET();
	else if(pGPIOx == GPIOG) GPIOG_REG_RESET();
	else if(pGPIOx == GPIOH) GPIOH_REG_RESET();
	else if(pGPIOx == GPIOI) GPIOI_REG_RESET();
}

/*
 * Data Read and Write
 */
uint8_t GPIO_ReadFromInputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber)
{
	uint8_t value;
	value = (uint8_t)((pGPIOx->IDR >> PinNumber) & 0x1);
	return value;
}

uint16_t GPIO_ReadFromInputPort(GPIO_RegDef_t *pGPIOx)
{
	uint16_t value;
	value = (uint16_t)pGPIOx->IDR;
	return value;
}

void GPIO_WriteToOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber, uint8_t Value)
{
	if(Value == GPIO_PIN_SET)
	{
		pGPIOx->ODR |= ( 1 << PinNumber);	// write 1
	} else
	{
		pGPIOx->ODR &= ~( 1 << PinNumber);	// write 0
	}
}

void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx, uint16_t Value)
{
	pGPIOx->ODR = Value;
}

void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber)
{
	pGPIOx->ODR  ^= ( 1 << PinNumber);
}


/*
 * IRQ configuration and ISR handling
 */
void GPIO_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{

	if(EnorDi == ENABLE)
	{
		if(IRQNumber <= 31)
		{
			//program ISER0 register
			*NVIC_ISER0 |= ( 1 << IRQNumber );

		}else if(IRQNumber > 31 && IRQNumber < 64 ) //32 to 63
		{
			//program ISER1 register
			*NVIC_ISER1 |= ( 1 << (IRQNumber % 32) );
		}
		else if(IRQNumber >= 64 && IRQNumber < 96 )
		{
			//program ISER2 register //64 to 95
			*NVIC_ISER2 |= ( 1 << (IRQNumber % 64) );
		}
	}else
	{
		if(IRQNumber <= 31)
		{
			//program ICER0 register
			*NVIC_ICER0 |= ( 1 << IRQNumber );
		}else if(IRQNumber > 31 && IRQNumber < 64 )
		{
			//program ICER1 register
			*NVIC_ICER1 |= ( 1 << (IRQNumber % 32) );
		}
		else if(IRQNumber >= 64 && IRQNumber < 96 )
		{
			//program ICER2 register
			*NVIC_ICER2 |= ( 1 << (IRQNumber % 64) );
		}
	}
}

void GPIO_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
	uint8_t iprx = IRQNumber / 4;
	uint8_t iprx_section = IRQNumber % 4;

	uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);

	*(NVIC_PR_BASE_ADDR + iprx) |= (IRQPriority << shift_amount);
}


/*********************************************************************
 * @fn      		  - GPIO_IRQHandling
 *
 * @brief             -
 *
 * @param[in]         -
 * @param[in]         -
 * @param[in]         -
 *
 * @return            -
 *
 * @Note              -

 */
void GPIO_IRQHandling(uint8_t PinNumber)
{
	// clear the exti pr register corresponding to the pin number
	if(EXTI->PR & (1 << PinNumber))
	{
		// clear
		EXTI->PR |= (1 << PinNumber);
	}
}









/*
 * 006spi_tx_testing.c
 *
 *  Created on: 25-Jun-2026
 *      Author: manna
 */


/*
 * From datasheet alternative functionality mapping table
 * PB14 ---> SPI2_MISO
 * PB15 ---> SPI2_MOSI
 * PB13 ---> SPI2_SCLK
 * PB12 ---> SPI_NSS
 * ALT function mode : 5
 */

#include <string.h>
#include "stm32f407xx.h"

void SPI2_GPIOInits(void)
{
	GPIO_Handle_t SPIPins;

	SPIPins.pGPIOx = GPIOB;
	SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	SPIPins.GPIO_PinConfig.GPIO_PinAltFunMode = 5;
	SPIPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	SPIPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
	SPIPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	// SCLK
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_13;
	GPIO_Init(&SPIPins);

	// MOSI
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_15;
	GPIO_Init(&SPIPins);

	// MISO
	//SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_14;
	//GPIO_Init(&SPIPins);

	// NSS
	//SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_12;
	//GPIO_Init(&SPIPins);
}

void SPI2_Inits(void)
{
	SPI_Handle_t SPI2handle;

	SPI2handle.pSPIx = SPI2;
	SPI2handle.SPIConfig.SPI_BusConfig = SPI_BUS_CONFIG_FD;
	SPI2handle.SPIConfig.SPI_DeviceMode = SPI_DEVICE_MODE_MASTER;
	SPI2handle.SPIConfig.SPI_SclkSpeed = SPI_SCLK_SPEED_DIV2;   // generates sclk of 8MHz
	SPI2handle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;
	SPI2handle.SPIConfig.SPI_CPOL = SPI_CPOL_HIGH;
	SPI2handle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;
	SPI2handle.SPIConfig.SPI_SSM = 	SPI_SSM_EN;		// software slave management enable for NSS pin

	SPI_Init(&SPI2handle);
}

int main(void)
{
	char user_data[] = "Hello World";


	// This function is used to initialize the GPIO pins to behave as SPI2 pins
	SPI2_GPIOInits();

	// This function is used to initialize the SPI2 peripheral parameters
	SPI2_Inits();

	// NOTE: All SPI configuration (CR1, CR2) MUST be done before enabling the peripheral.
	// Once SPE = 1, SPI is active and CR1/CR2 registers become read-only.

	SPI_SSIConfig(SPI2, ENABLE);  // this makes NSS signal internally high and avoid MODF error

	// ─----------- Ensure SSI = 1 to prevent Mode Fault (MODF) ──────────────────────
	//
	// Problem:
	//   After enabling SPI, the MSTR bit was unexpectedly cleared (master → slave).
	//   Checking the Status Register (SR) revealed MODF = 1.
	//
	// Root cause (from STM32 RM, SPI section):
	//   MODF (Master Mode Fault) is triggered when the hardware detects an NSS conflict.
	//   It fires under two conditions:
	//     i)  NSS pin is pulled LOW          → in Hardware NSS mode  (SSM = 0)
	//     ii) SSI bit is 0                   → in Software NSS mode  (SSM = 1)
	//   When MODF fires, hardware auto-clears MSTR as a bus-protection measure.
	//
	// Why SSI matters in Software NSS mode (SSM = 1):
	//   In SSM mode, the physical NSS pin is ignored.
	//   Instead, the SSI bit acts as an internal virtual NSS line:
	//     SSI = 0  →  internal NSS = LOW   →  hardware sees a multi-master conflict
	//                                          →  MODF fires  →  MSTR cleared
	//     SSI = 1  →  internal NSS = HIGH  →  no conflict detected  →  stable master
	//
	// SSI defaults to 0 on reset, so without explicitly setting it,
	// MODF will always fire when SSM = 1.
	//
	// Fix: Set SSI = 1 before enabling SPE.
	//   This internally ties NSS HIGH, equivalent to connecting the NSS pin
	//   to VCC in hardware mode — signalling "I am the sole master on this bus."

	// Enable the SPI2 peripheral
	SPI_PeripheralControl(SPI2, ENABLE);

	// to send data
	SPI_SendData(SPI2, (uint8_t*)user_data, strlen(user_data));

	// lets confirm SPI is not busy
	while( SPI_GetFlagStatus(SPI2, SPI_BUSY_FLAG) );

	// disable the SPI2 peripheral
	SPI_PeripheralControl(SPI2, DISABLE);


	while(1);

	return 0;
}

/*
 * @brief I2CM bus slave example using interrupt mode
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "board.h"

#include "queue.h"
/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/


/* I2C clock is set to 1.8MHz */
#define I2C_CLK_DIVIDER     (40)

/* 100KHz I2C bit-rate - going too fast may prevent the salev from responding
   in time */
#define I2C_BITRATE         (100000)
/* Standard I2C mode */
#define I2C_MODE    (0)

/** Our slave address and I2C information */
#define LPC_I2C_PORT         LPC_I2C
#define LPC_I2C_INTHAND      I2C_IRQHandler
#define LPC_IRQNUM           I2C_IRQn

#define	LED_RED					0
#define	LED_GREEN				1
#define	LED_BLUE				2

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/* Converts given value @a val into a hexadecimal string
 * and stores the result to @a dest with leading zeros
 * RETURN: Number of hexdigits excluding leading zeros
 */
static int Hex2Str(char *dest, uint32_t val)
{
	int i, ret = 0;
	for (i = 0; i < sizeof(val) * 2; i ++) {
		int idx = val & 0xF;
		dest[7 - i] = "0123456789ABCDEF"[idx];
		val >>= 4;
		if (idx)
			ret = i;
	}
	return ret + 1;
}

/* Prints a hexadecimal value with given string in front */
/* Using printf might cause text section overflow */
static void Print_Val(const char *str, uint32_t val)
{
	char buf[9];
	int ret;
	buf[8] = 0;
	DEBUGSTR(str);
	ret = Hex2Str(buf, val);
	DEBUGSTR(&buf[4]);
	//DEBUGSTR(&buf[8 - ret]);
}

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Initializes pin muxing for I2C interface - note that SystemInit() may
   already setup your pin muxing at system startup */
static void Init_I2C_PinMux(void)
{
#if (defined(BOARD_NXP_LPCXPRESSO_812) || defined(BOARD_LPC812MAX) || defined(BOARD_NXP_LPCXPRESSO_824))
	/* Enable the clock to the Switch Matrix */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);

#if defined(BOARD_NXP_LPCXPRESSO_824)
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SDA);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SCL);
#else
	/* Connect the I2C_SDA and I2C_SCL signals to port pins(P0.10, P0.11) */
	Chip_SWM_MovablePinAssign(SWM_I2C_SDA_IO, 10);
	Chip_SWM_MovablePinAssign(SWM_I2C_SCL_IO, 11);
#endif

	/* Enable Fast Mode Plus for I2C pins */
	Chip_IOCON_PinSetI2CMode(LPC_IOCON, IOCON_PIO10, PIN_I2CMODE_FASTPLUS);
	Chip_IOCON_PinSetI2CMode(LPC_IOCON, IOCON_PIO11, PIN_I2CMODE_FASTPLUS);

	/* Disable the clock to the Switch Matrix to save power */
	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);

#else
	/* Configure your own I2C pin muxing here if needed */
#warning "No I2C pin muxing defined"
#endif
}

/* Setup I2C */
static void setupI2CSlave(void)
{
	/* Some common I2C init was performed in setupI2CMaster(), so it doesn't
	   need to be done again for the slave setup. */



	/* Clear interrupt status and enable slave interrupts */
	Chip_I2CS_ClearStatus(LPC_I2C_PORT, I2C_STAT_SLVDESEL);
	Chip_I2C_EnableInt(LPC_I2C_PORT, I2C_INTENSET_SLVPENDING | I2C_INTENSET_SLVDESEL);

	/* Enable I2C slave interface */
	Chip_I2CS_Enable(LPC_I2C_PORT);
}

static void Chip_I2C_Monitor_Enable(LPC_I2C_T *pI2C)
{
	pI2C->CFG = (pI2C->CFG & I2C_CFG_MASK) | I2C_CFG_MONEN ;
}


static void Chip_I2C_ClearStatus(LPC_I2C_T *pI2C, uint32_t clrStatus)
{

	pI2C->STAT =  clrStatus;
}
STATIC INLINE uint16_t Chip_I2C_Monitor_GetData(LPC_I2C_T *pI2C)
{
	return pI2C->MONRXDAT;
}

static void setupI2CMonitor(void)
{
	/* Enable I2C clock and reset I2C peripheral */
	Chip_I2C_Init(LPC_I2C_PORT);

	/* Clear Monitor status and enable Monitor interrupts */
	Chip_I2CS_ClearStatus(LPC_I2C_PORT, I2C_STAT_MONOV | I2C_STAT_MONIDLE);

    /*
	 * Monitor Ready Interrupt Enable
	 * Monitor Overflow Interrupt Enable
	 * Monitor Idle Interrupt Enable
	 */
	Chip_I2C_EnableInt(LPC_I2C_PORT, I2C_INTENSET_MONRDY | I2C_INTENSET_MONOV | I2C_INTENSET_MONIDLE);

	/* Enable I2C slave interface */
	Chip_I2C_Monitor_Enable(LPC_I2C_PORT);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle I2C0 interrupt by calling I2CM interrupt transfer handler
 * @return	Nothing
 */
void LPC_I2C_INTHAND(void)
{
	uint32_t state = Chip_I2C_GetPendingInt(LPC_I2C_PORT);

	if (state & (I2C_INTSTAT_MONRDY)) {
		QAdd(Chip_I2C_Monitor_GetData(LPC_I2C_PORT));
		Board_LED_Toggle(LED_RED);
	}
	if (state & (I2C_INTSTAT_MONOV)) {
		Chip_I2C_ClearStatus(LPC_I2C_PORT,I2C_STAT_MONOV);
		Board_LED_Toggle(LED_GREEN);
	}
	if (state & (I2C_INTSTAT_MONIDLE)) {
		Chip_I2C_ClearStatus(LPC_I2C_PORT,I2C_STAT_MONIDLE);
		Board_LED_Toggle(LED_BLUE);
	}
}


void SysTick_Handler(void)
{

}
/**
 * @brief	Main routine for I2C example
 * @return	Function should not exit
 */

int main(void)
{
	/* Generic Initialization */
	SystemCoreClockUpdate();
	Board_Init();

	/* Clear activity LED */
	Board_LED_Set(0, false);

	/* Setup I2C pin muxing */
	Init_I2C_PinMux();

	/* Setup I2C, master, and slave */
	setupI2CMonitor();

	/* Enable the interrupt for the I2C */
	NVIC_EnableIRQ(LPC_IRQNUM);

	DEBUGSTR("2 Emulated EEPROM I2C devices using 2 I2C slaves\r\n");

	uint16_t get_item = 0;
	/* Test each emulated EEPROM */
	while (1) {
		/* Read some data from the emulated EEPROMs */

		if(! (QisEmpty() && flag == 0))
		{

			if(QGet(&get_item))
			{
				Print_Val(" ",get_item);
			}
			if(get_item & 0x0400)
			{
				Board_UARTPutSTR("\r\n");

//				Print_Val("Buff Length 0x",QBuffLength());
			}
//			Board_UARTPutSTR("\r\n");
		}
	}
}

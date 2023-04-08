/**
  ******************************************************************************
  * @file           : I2cHdlr.c
  * @brief          : I2c Handler
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 EmbeddedEspresso.
  * All rights reserved.
  *
  * This software component is licensed by EmbeddedEspresso under BSD 3-Clause
  * license. You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  * opensource.org/licenses/BSD-3-Clause
  ******************************************************************************
  */

#include "main.h"

#define I2cHdlrSetReset(m)			(m->CR1 |= 0x8000)
#define I2cHdlrClrReset(m)			(m->CR1 &= ~0x8000)
#define I2cHdlrSetClockFreq(m, f)	(m->CR2 = (f))
#define I2cHdlrSetTRise(m, f)		(m->TRISE = (f))
#define I2cHdlrSetClockConf(m, c)   (m->CCR = (c))
#define I2cHdlrSetOAR1(m, c)		(m->OAR1 = (c))
#define I2cHdlrEnableAck(m)			(m->CR1 |= 0x0400)
#define I2cHdlrDisableAck(m)		(m->CR1 &= ~0x0400)
#define I2cHdlrEnablePeri(m)		(m->CR1 |= 0x0001)
#define I2cHdlrClrPec(m)			(m->CR1 &= ~0x0800)
#define I2cHdlrSendStart(m) 		(m->CR1 |= 0x0100)
#define I2cHdlrIsStartSent(m)		(((m->SR1) & 0x01) == 0x01)
#define I2cHdlrSendData(m, d)		(m->DR = d)
#define I2cHdlrReceiveData(m)		(m->DR)
#define I2cHdlrIsAddrSent(m)		(((m->SR1) & 0x02) == 0x02)
#define I2cHdlrIsNackRec(m)			(((m->SR1) & 0x0400) == 0x0400)
#define I2cHdlrClrNack(m)			((m->SR1) &= ~0x0400)
#define I2cHdlrSendStop(m)			(m->CR1 |= 0x0200)
#define I2cHdlrIsTxRegEmpty(m)		((m->SR1 & 0x80) != 0)
#define I2cHdlrIsRxRegNotEmpty(m)	((m->SR1 & 0x40) != 0)

#define I2C_MAX_DEVICE_NUM 2

typedef struct
{
    volatile uint32_t CR1;        /*!< I2C Control register 1,     Address offset: 0x00 */
    volatile uint32_t CR2;        /*!< I2C Control register 2,     Address offset: 0x04 */
    volatile uint32_t OAR1;       /*!< I2C Own address register 1, Address offset: 0x08 */
    volatile uint32_t OAR2;       /*!< I2C Own address register 2, Address offset: 0x0C */
    volatile uint32_t DR;         /*!< I2C Data register,          Address offset: 0x10 */
    volatile uint32_t SR1;        /*!< I2C Status register 1,      Address offset: 0x14 */
    volatile uint32_t SR2;        /*!< I2C Status register 2,      Address offset: 0x18 */
    volatile uint32_t CCR;        /*!< I2C Clock control register, Address offset: 0x1C */
    volatile uint32_t TRISE;      /*!< I2C TRISE register,         Address offset: 0x20 */
    volatile uint32_t FLTR;       /*!< I2C FLTR register,          Address offset: 0x24 */
} tI2cRegMap;

typedef enum
{
    I2C_HDLR_INIT = 0,
    I2C_HDLR_IDLE,
    I2C_HDLR_DATATX,
	I2C_HDLR_DATARX
} tI2cHdlrFsmSts;

typedef enum
{
    I2C_HDLR_TX_IDLE = 0,
    I2C_HDLR_TX_STARTTX,
	I2C_HDLR_TX_SENDADDR,
	I2C_HDLR_TX_CHECKADDR,
	I2C_HDLR_TX_SENDDATA,
	I2C_HDLR_TX_SENDDATA_WAIT,
} tI2cHdlrTxFsmSts;

typedef enum
{
    I2C_HDLR_RX_IDLE = 0,
    I2C_HDLR_RX_STARTRX,
	I2C_HDLR_RX_SENDADDR,
	I2C_HDLR_RX_CHECKADDR,
	I2C_HDLR_RX_RECDATA,
	I2C_HDLR_RX_RECDATA_WAIT,
} tI2cHdlrRxFsmSts;

typedef struct
{
    uint8_t *pData;
    uint32_t length;
    uint8_t addr;
    uint8_t currPos;
} tI2cHdlrCurrTr;

typedef struct
{
	tI2cHdlrFsmSts fsmsts;
	tI2cHdlrTxFsmSts fsmtxsts;
	tI2cHdlrRxFsmSts fsmrxsts;
	tI2cHdlrCurrTr currTr;
	tI2cRegMap *regMap;
} tI2cHdlrInstance;

uint32_t i2cBaseAddr[] = {I2C1_BASE, I2C2_BASE, I2C3_BASE};

/* 3 I2cs are present on the device */
static tI2cHdlrInstance i2cHdlrInst[3];

void I2cHdlrInit (void)
{
	uint32_t idx;

    for (idx=0; idx<I2C_MAX_DEVICE_NUM; idx++)
    {
		i2cHdlrInst[idx].fsmsts = I2C_HDLR_INIT;
		i2cHdlrInst[idx].fsmtxsts = I2C_HDLR_TX_IDLE;
		i2cHdlrInst[idx].fsmrxsts = I2C_HDLR_RX_IDLE;
		i2cHdlrInst[idx].regMap = i2cBaseAddr[idx];

		I2cHdlrSetReset(i2cHdlrInst[idx].regMap);
		I2cHdlrClrReset(i2cHdlrInst[idx].regMap);
		I2cHdlrSetClockFreq(i2cHdlrInst[idx].regMap, 0x24u);
		I2cHdlrSetTRise(i2cHdlrInst[idx].regMap, 0x25u);
		I2cHdlrSetClockConf(i2cHdlrInst[idx].regMap, 0xb4u);
		I2cHdlrSetOAR1(i2cHdlrInst[idx].regMap, 0x4000);

		I2cHdlrEnablePeri(i2cHdlrInst[idx].regMap);
    }

    PRINT_DEBUG("[I2c]: Initialization completed\r\n");
}

void I2cHdlrRun (void)
{
    static int idx = 0u;
    I2cHdlrErrCode trResult;

    for (idx=0; idx<I2C_MAX_DEVICE_NUM; idx++)
    {
		switch (i2cHdlrInst[idx].fsmsts)
		{
			case I2C_HDLR_INIT:
				/* Start with the initialization */
				/* Copy here */
				i2cHdlrInst[idx].fsmsts = I2C_HDLR_IDLE;
				break;

			case I2C_HDLR_IDLE:

				break;

			case I2C_HDLR_DATATX:
				trResult = I2cHdlrTxRun(idx);
				if (trResult == I2C_HDLR_OK)
				{
					i2cHdlrInst[idx].fsmtxsts = I2C_HDLR_TX_IDLE;
					i2cHdlrInst[idx].fsmsts = I2C_HDLR_IDLE;
				}
				if (trResult == I2C_HDLR_ERR)
				{
					i2cHdlrInst[idx].fsmtxsts = I2C_HDLR_TX_IDLE;
					i2cHdlrInst[idx].fsmsts = I2C_HDLR_IDLE;
				}

				break;

			case I2C_HDLR_DATARX:
				trResult = I2cHdlrRxRun(idx);
				if (trResult == I2C_HDLR_OK)
				{
					i2cHdlrInst[idx].fsmrxsts = I2C_HDLR_RX_IDLE;
					i2cHdlrInst[idx].fsmsts = I2C_HDLR_IDLE;
				}
				if (trResult == I2C_HDLR_ERR)
				{
					i2cHdlrInst[idx].fsmrxsts = I2C_HDLR_RX_IDLE;
					i2cHdlrInst[idx].fsmsts = I2C_HDLR_IDLE;
				}

				break;

		}
    }

}

I2cHdlrErrCode I2cHdlrTxRun (tI2cHdlrModIdx devIdx)
{
	I2cHdlrErrCode result = I2C_HDLR_BUSY;
	static uint32_t currPos;
	uint16_t tempreg;

	switch (i2cHdlrInst[devIdx].fsmtxsts)
	{
		case I2C_HDLR_TX_IDLE:
			result = I2C_HDLR_OK;
			break;

		case I2C_HDLR_TX_STARTTX:
		    I2cHdlrEnableAck(i2cHdlrInst[devIdx].regMap);
            /* Generate Start condition */
			I2cHdlrSendStart(i2cHdlrInst[devIdx].regMap);
            /* Wait start condition generation detection */
			i2cHdlrInst[devIdx].fsmtxsts = I2C_HDLR_TX_SENDADDR;
            /* Send address */
            break;

		case I2C_HDLR_TX_SENDADDR:
			/* Check if start condition has been sent */
			if (I2cHdlrIsStartSent(i2cHdlrInst[devIdx].regMap))
			{
				I2cHdlrSendData(i2cHdlrInst[devIdx].regMap, i2cHdlrInst[devIdx].currTr.addr);
				/* Wait address sent condition generation detection */
				i2cHdlrInst[devIdx].fsmtxsts = I2C_HDLR_TX_CHECKADDR;
			}
			break;

		case I2C_HDLR_TX_CHECKADDR:
			/* Check if address has been sent */
			if (I2cHdlrIsAddrSent(i2cHdlrInst[devIdx].regMap))
			{
				/* This sequence, cleares the ADDR bit */
				/* To be macrofied */
				tempreg = i2cHdlrInst[devIdx].regMap->SR1;
				tempreg = i2cHdlrInst[devIdx].regMap->SR2;

				/* Check of NAK */
				if (I2cHdlrIsNackRec(i2cHdlrInst[devIdx].regMap))
				{
					/* Generate Stop */
					I2cHdlrSendStop(i2cHdlrInst[devIdx].regMap);
					/* Cleat ack failure */
					I2cHdlrClrNack(i2cHdlrInst[devIdx].regMap);

					result = I2C_HDLR_ERR;
				}
				i2cHdlrInst[devIdx].currTr.currPos = 0u;
				/* Wait address sent condition generation detection */
				i2cHdlrInst[devIdx].fsmtxsts = I2C_HDLR_TX_SENDDATA;

            }
			break;

		case I2C_HDLR_TX_SENDDATA:
            /* Send data */
			I2cHdlrSendData(i2cHdlrInst[devIdx].regMap, i2cHdlrInst[devIdx].currTr.pData[i2cHdlrInst[devIdx].currTr.currPos]);
			i2cHdlrInst[devIdx].currTr.currPos++;
			i2cHdlrInst[devIdx].fsmtxsts = I2C_HDLR_TX_SENDDATA_WAIT;
			break;

		case I2C_HDLR_TX_SENDDATA_WAIT:

			if(!I2cHdlrIsTxRegEmpty(i2cHdlrInst[devIdx].regMap))
			{
				/* Wait TXE to be set*/
				if (I2cHdlrIsNackRec(i2cHdlrInst[devIdx].regMap))
				{
					/* Generate Stop */
					I2cHdlrSendStop(i2cHdlrInst[devIdx].regMap);
					/* Cleat ack failure */
					I2cHdlrClrNack(i2cHdlrInst[devIdx].regMap);

					result = I2C_HDLR_ERR;

				}
			}
			else
			{
				if (i2cHdlrInst[devIdx].currTr.currPos == i2cHdlrInst[devIdx].currTr.length)
				{
					result = I2C_HDLR_OK;
					/* Data transfer is finished */
					i2cHdlrInst[devIdx].fsmtxsts = I2C_HDLR_TX_IDLE;
					/* Generate Stop */
					I2cHdlrSendStop(i2cHdlrInst[devIdx].regMap);
				}
				else
				{
					i2cHdlrInst[devIdx].fsmtxsts = I2C_HDLR_TX_SENDDATA;
				}
			}
			break;
	}

	return result;
}

I2cHdlrErrCode I2cHdlrRxRun (tI2cHdlrModIdx devIdx)
{
	I2cHdlrErrCode result = I2C_HDLR_BUSY;
	static uint32_t currPos;
	uint16_t tempreg;

	switch (i2cHdlrInst[devIdx].fsmrxsts)
	{
		case I2C_HDLR_RX_IDLE:
			result = I2C_HDLR_OK;
			break;

		case I2C_HDLR_RX_STARTRX:
		    I2cHdlrEnableAck(i2cHdlrInst[devIdx].regMap);
            /* Generate Start condition */
			I2cHdlrSendStart(i2cHdlrInst[devIdx].regMap);
            /* Wait start condition generation detection */
			i2cHdlrInst[devIdx].fsmrxsts = I2C_HDLR_RX_SENDADDR;
            /* Send address */
            break;

		case I2C_HDLR_RX_SENDADDR:
			/* Check if start condition has been sent */
			if (I2cHdlrIsStartSent(i2cHdlrInst[devIdx].regMap))
			{
				I2cHdlrSendData(i2cHdlrInst[devIdx].regMap, (i2cHdlrInst[devIdx].currTr.addr | 0x01));
				/* Wait address sent condition generation detection */
				i2cHdlrInst[devIdx].fsmrxsts = I2C_HDLR_RX_CHECKADDR;
			}
			break;

		case I2C_HDLR_RX_CHECKADDR:
			/* Check if address has been sent */
			if (I2cHdlrIsAddrSent(i2cHdlrInst[devIdx].regMap))
			{
				if (i2cHdlrInst[devIdx].currTr.length == 1u)
				{
					/* A Nack must be set for the last byte to stop comm */
					I2cHdlrDisableAck(i2cHdlrInst[devIdx].regMap);
				}

				/* This sequence, cleares the ADDR bit */
				/* To be macrofied */
				tempreg = i2cHdlrInst[devIdx].regMap->SR1;
				tempreg = i2cHdlrInst[devIdx].regMap->SR2;

				if (i2cHdlrInst[devIdx].currTr.length == 1u)
				{
					/* Generate Stop */
					I2cHdlrSendStop(i2cHdlrInst[devIdx].regMap);
				}

				i2cHdlrInst[devIdx].currTr.currPos = 0u;
				/* Wait address sent condition generation detection */
				i2cHdlrInst[devIdx].fsmrxsts = I2C_HDLR_RX_RECDATA;

            }
			break;

		case I2C_HDLR_RX_RECDATA:
            if(I2cHdlrIsRxRegNotEmpty(i2cHdlrInst[devIdx].regMap))
            {
            	i2cHdlrInst[devIdx].currTr.pData[i2cHdlrInst[devIdx].currTr.currPos] = I2cHdlrReceiveData(i2cHdlrInst[devIdx].regMap);
            	i2cHdlrInst[devIdx].currTr.currPos++;

				if (i2cHdlrInst[devIdx].currTr.currPos == i2cHdlrInst[devIdx].currTr.length)
				{
					result = I2C_HDLR_OK;
					/* Data transfer is finished */
					i2cHdlrInst[devIdx].fsmrxsts = I2C_HDLR_TX_IDLE;
				}
				else if(i2cHdlrInst[devIdx].currTr.currPos == (i2cHdlrInst[devIdx].currTr.length - 1u))
				{
					/* A Nack must be set for the last byte to stop comm */
					I2cHdlrDisableAck(i2cHdlrInst[devIdx].regMap);
					/* Generate Stop */
					I2cHdlrSendStop(i2cHdlrInst[devIdx].regMap);
				}
            }
			break;

	}

	return result;
}

boolean I2cHdlrIsFsmBusy (tI2cHdlrModIdx devIdx)
{
	boolean isBusy = TRUE;

	if (i2cHdlrInst[devIdx].fsmsts == I2C_HDLR_IDLE)
	{
		isBusy = FALSE;
	}

	return isBusy;
}

I2cHdlrErrCode I2cHdlrMasterTx (tI2cHdlrModIdx devIdx, uint8_t addr, uint8_t *data, uint16_t length)
{
	I2cHdlrErrCode result = I2C_HDLR_BUSY;

	if ( (i2cHdlrInst[devIdx].fsmsts == I2C_HDLR_IDLE) &&
		 (i2cHdlrInst[devIdx].fsmtxsts == I2C_HDLR_TX_IDLE) )
	{
		i2cHdlrInst[devIdx].currTr.addr = addr;
		i2cHdlrInst[devIdx].currTr.length = length;
		i2cHdlrInst[devIdx].currTr.pData = data;
		i2cHdlrInst[devIdx].fsmsts = I2C_HDLR_DATATX;
		i2cHdlrInst[devIdx].fsmtxsts = I2C_HDLR_TX_STARTTX;
		result = I2C_HDLR_OK;
	}

	return result;
}

I2cHdlrErrCode I2cHdlrMasterRx (tI2cHdlrModIdx devIdx, uint8_t addr, uint8_t *data, uint16_t length)
{
	I2cHdlrErrCode result = I2C_HDLR_BUSY;

	if ( (i2cHdlrInst[devIdx].fsmsts == I2C_HDLR_IDLE) &&
		 (i2cHdlrInst[devIdx].fsmrxsts == I2C_HDLR_RX_IDLE) )
	{
		i2cHdlrInst[devIdx].currTr.addr = addr;
		i2cHdlrInst[devIdx].currTr.length = length;
		i2cHdlrInst[devIdx].currTr.pData = data;
		i2cHdlrInst[devIdx].fsmsts = I2C_HDLR_DATARX;
		i2cHdlrInst[devIdx].fsmrxsts = I2C_HDLR_RX_STARTRX;
		result = I2C_HDLR_OK;
	}
	return result;
}

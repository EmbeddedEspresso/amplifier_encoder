/**
  ******************************************************************************
  * @file           : EncHdlr.c
  * @brief          : Encoder Handler
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

#define ENC_CFG_LENGTH 6u

typedef enum
{
    ENC_HDLR_INIT = 0,
	ENC_HDLR_CFG,
	ENC_HDLR_CFG_WAIT,
	ENC_HDLR_PREIDLE,
	ENC_HDLR_IDLE,
	ENC_HDLR_GETSTSTX,
	ENC_HDLR_GETSTSTX_WAIT,
	ENC_HDLR_GETSTSRX,
	ENC_HDLR_GETSTSRX_WAIT,
	ENC_HDLR_GETPOSTX,
	ENC_HDLR_GETPOSTX_WAIT,
	ENC_HDLR_GETPOSRX,
	ENC_HDLR_GETPOSRX_WAIT
} tEncHdlrFsmSts;

typedef struct
{
	uint8_t cnf[5];
	uint32_t length;
}tEncHdlrCfg;

static tEncHdlrFsmSts fsmsts;
static uint16_t devAddress = 0x8E;
/* Provide a configuration struct */
/* Improvement: Use the INT pin to trigger encoder read */

static uint8_t encValConfRegAddr = 0x0C;
static tEncHdlrCfg encRegConf[ENC_CFG_LENGTH] = { { {0x04u, 0x18u}, 0x02u },
												  { {0x08u, 0x00u, 0x00u, 0x00u, 0x06u}, 0x05u },
												  { {0x0Cu, 0x00u, 0x00u, 0x00u, 0x10u}, 0x05u },
											      { {0x10u, 0x00u, 0x00u, 0x00u, 0x00u}, 0x05u },
											      { {0x14u, 0x00u, 0x00u, 0x00u, 0x01u}, 0x05u },
											      { {0x30u, 0x00u, 0x00u, 0x00u, 0x01u}, 0x05u },
												};

static uint8_t encValPosRegAddr = 0x08;
static uint8_t encStsPosRegAddr = 0x05;
static uint8_t dataReg[4] = {0};
static char debugLocalStr[256];
static uint8_t encVal = 6u;

EncHdlrErrCode EncHdlrInit (void)
{
	fsmsts = ENC_HDLR_INIT;
}

EncHdlrErrCode EncHdlrRun (void)
{
	EncHdlrErrCode result = ENC_HDLR_OK;
	static int tmr;
    static int idx = 0u;
    static int cfgIdx = 0u;

    float tempVal = 0;
    float tempValDec;


	switch (fsmsts)
	{
		case ENC_HDLR_INIT:
			/* Start with the initialization */
			/* Copy here */
			fsmsts = ENC_HDLR_CFG;
			break;

		case ENC_HDLR_CFG:
			if (I2cHdlrMasterTx(I2C_HDLR_MOD1, devAddress, encRegConf[cfgIdx].cnf, encRegConf[cfgIdx].length) == ENC_HDLR_OK)
			{
				fsmsts = ENC_HDLR_CFG_WAIT;
			}
			break;

		case ENC_HDLR_CFG_WAIT:
			if (I2cHdlrIsFsmBusy(I2C_HDLR_MOD1) == FALSE)
			{
				cfgIdx++;
				if (cfgIdx < ENC_CFG_LENGTH)
				{
					fsmsts = ENC_HDLR_CFG;
				}
				else
				{
					fsmsts = ENC_HDLR_PREIDLE;
				}
	            TimerSet(&tmr, 600);
			}
			break;

		case ENC_HDLR_PREIDLE:
            PRINT_DEBUG("[Encoder]: Initialization completed\r\n");
            fsmsts = ENC_HDLR_IDLE;
            break;

		case ENC_HDLR_IDLE:
			if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10) == GPIO_PIN_RESET )
			{
				fsmsts = ENC_HDLR_GETSTSTX;
			}
			break;

		case ENC_HDLR_GETSTSTX:
			if (I2cHdlrMasterTx(I2C_HDLR_MOD1, devAddress, &encStsPosRegAddr, 1) == I2C_HDLR_OK)
			{
				fsmsts = ENC_HDLR_GETSTSTX_WAIT;
			}
			break;

		case ENC_HDLR_GETSTSTX_WAIT:
			if (I2cHdlrIsFsmBusy(I2C_HDLR_MOD1) == FALSE)
			{
				fsmsts = ENC_HDLR_GETSTSRX;
			}
			break;

		case ENC_HDLR_GETSTSRX:
			if (I2cHdlrMasterRx(I2C_HDLR_MOD1, devAddress, dataReg, 1) == I2C_HDLR_OK)
			{
				fsmsts = ENC_HDLR_GETSTSRX_WAIT;
			}
			break;

		case ENC_HDLR_GETSTSRX_WAIT:
			if (I2cHdlrIsFsmBusy(I2C_HDLR_MOD1) == FALSE)
			{
				fsmsts = ENC_HDLR_GETPOSTX;
			}
			break;

		case ENC_HDLR_GETPOSTX:
			if (I2cHdlrMasterTx(I2C_HDLR_MOD1, devAddress, &encValPosRegAddr, 1) == I2C_HDLR_OK)
			{
				fsmsts = ENC_HDLR_GETPOSTX_WAIT;
			}
			break;

		case ENC_HDLR_GETPOSTX_WAIT:
			if (I2cHdlrIsFsmBusy(I2C_HDLR_MOD1) == FALSE)
			{
				fsmsts = ENC_HDLR_GETPOSRX;
			}
			break;

		case ENC_HDLR_GETPOSRX:
			if (I2cHdlrMasterRx(I2C_HDLR_MOD1, devAddress, dataReg, 4) == I2C_HDLR_OK)
			{
				fsmsts = ENC_HDLR_GETPOSRX_WAIT;
			}
			break;

		case ENC_HDLR_GETPOSRX_WAIT:
			if (I2cHdlrIsFsmBusy(I2C_HDLR_MOD1) == FALSE)
			{
                sprintf(debugLocalStr, "[Encoder]: Encoder value: %d\r\n", dataReg[3]);
				PRINT_DEBUG(debugLocalStr);
				if (encVal != dataReg[3])
				{
					encVal = dataReg[3];
					AmpHdlrSetGain(encVal);
				}
				fsmsts = ENC_HDLR_IDLE;
	            TimerSet(&tmr, 600);
			}
			break;

	}

	return result;
}


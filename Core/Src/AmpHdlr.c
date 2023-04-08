/**
  ******************************************************************************
  * @file           : AmpHdlr.c
  * @brief          : Amplifier Handler
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

#define AMP_CFG_LENGTH 2u

typedef enum
{
    AMP_HDLR_INIT = 0,
	AMP_HDLR_CFG,
	AMP_HDLR_CFG_WAIT,
	AMP_HDLR_PREIDLE,
	AMP_HDLR_IDLE,
	AMP_HDLR_GETGAINTX,
	AMP_HDLR_GETGAINTX_WAIT,
	AMP_HDLR_GETGAINRX,
	AMP_HDLR_GETGAINRX_WAIT,
	AMP_HDLR_SETGAINTX,
	AMP_HDLR_SETGAINTX_WAIT
} tAmpHdlrFsmSts;

typedef struct
{
	uint8_t cnf[5];
	uint32_t length;
}tAmpHdlrCfg;

static tAmpHdlrFsmSts fsmsts;
static uint16_t devAddress = 0xB0;
/* Provide a configuration struct */
/* Improvement: Use the INT pin to trigger encoder read */

static tAmpHdlrCfg ampRegConf[AMP_CFG_LENGTH] = { { {0x07u, 0xC0u}, 0x02u },
		 	 	 	 	 	 	 	 	 	 	  { {0x03u, 0x01u}, 0x02u }
												};


static uint8_volume;
static uint8_t isVolumeChanged;

static uint8_t ampValPosRegAddr = 0x05;
static tAmpHdlrCfg ampSetGainCmd = { {0x05u, 0x00u}, 0x02u };

static uint8_t dataReg[4] = {0};
static char debugLocalStr[256];

static uint8_t ampSetGain = 0u;
static uint8_t ampGain = 0u;

AmpHdlrErrCode AmpHdlrInit (void)
{
	fsmsts = AMP_HDLR_INIT;
}

AmpHdlrErrCode AmpHdlrRun (void)
{
	AmpHdlrErrCode result = AMP_HDLR_OK;
	static int tmr;
    static int idx = 0u;
    static int cfgIdx = 0u;

    float tempVal = 0;
    float tempValDec;


	switch (fsmsts)
	{
		case AMP_HDLR_INIT:
			/* Start with the initialization */
			/* Copy here */
			if (AMP_CFG_LENGTH > 0u)
			{
				fsmsts = AMP_HDLR_CFG;
			}
			else
			{
				fsmsts = AMP_HDLR_PREIDLE;
			}
			break;

		case AMP_HDLR_CFG:
			if (I2cHdlrMasterTx(I2C_HDLR_MOD2, devAddress, ampRegConf[cfgIdx].cnf, ampRegConf[cfgIdx].length) == AMP_HDLR_OK)
			{
				fsmsts = AMP_HDLR_CFG_WAIT;
			}
			break;

		case AMP_HDLR_CFG_WAIT:
			if (I2cHdlrIsFsmBusy(I2C_HDLR_MOD2) == FALSE)
			{
				cfgIdx++;
				if (cfgIdx < AMP_CFG_LENGTH)
				{
					fsmsts = AMP_HDLR_CFG;
				}
				else
				{
					fsmsts = AMP_HDLR_PREIDLE;
				}
	            TimerSet(&tmr, 1000);
			}
			break;

		case AMP_HDLR_PREIDLE:
            PRINT_DEBUG("[Amplifier]: Initialization completed\r\n");
            fsmsts = AMP_HDLR_IDLE;
            break;

		case AMP_HDLR_IDLE:
			if (ampSetGain == 1u)
			{
				ampSetGain = 0u;
				fsmsts = AMP_HDLR_SETGAINTX;
			}
			else
			{
				if (isTimerExpired(tmr))
				{
					fsmsts = AMP_HDLR_GETGAINTX;
				}
			}
			break;

		case AMP_HDLR_GETGAINTX:
			if (I2cHdlrMasterTx(I2C_HDLR_MOD2, devAddress, &ampValPosRegAddr, 1) == I2C_HDLR_OK)
			{
				fsmsts = AMP_HDLR_GETGAINTX_WAIT;
			}
			break;

		case AMP_HDLR_GETGAINTX_WAIT:
			if (I2cHdlrIsFsmBusy(I2C_HDLR_MOD2) == FALSE)
			{
				fsmsts = AMP_HDLR_GETGAINRX;
			}
			break;

		case AMP_HDLR_GETGAINRX:
			if (I2cHdlrMasterRx(I2C_HDLR_MOD2, devAddress, dataReg, 1) == I2C_HDLR_OK)
			{
				fsmsts = AMP_HDLR_GETGAINRX_WAIT;
			}
			break;

		case AMP_HDLR_GETGAINRX_WAIT:
			if (I2cHdlrIsFsmBusy(I2C_HDLR_MOD2) == FALSE)
			{
                sprintf(debugLocalStr, "[Amplifier]: Gain value: %d\r\n", dataReg[0]);
				PRINT_DEBUG(debugLocalStr);
				fsmsts = AMP_HDLR_IDLE;
	            TimerSet(&tmr, 1000);
			}
			break;

		case AMP_HDLR_SETGAINTX:
			ampSetGainCmd.cnf[1] = ampGain*2;
			if (I2cHdlrMasterTx(I2C_HDLR_MOD2, devAddress, ampSetGainCmd.cnf, ampSetGainCmd.length) == AMP_HDLR_OK)
			{
				fsmsts = AMP_HDLR_SETGAINTX_WAIT;
			}
			break;

		case AMP_HDLR_SETGAINTX_WAIT:
			if (I2cHdlrIsFsmBusy(I2C_HDLR_MOD2) == FALSE)
			{
				fsmsts = AMP_HDLR_IDLE;
	            TimerSet(&tmr, 1000);
	            PRINT_DEBUG("[Amplifier]: Gain updated\r\n");
			}
			break;

	}

	return result;
}

AmpHdlrErrCode AmpHdlrSetGain (uint8_t gain)
{
	ampSetGain = 1u;
	ampGain = gain;

	return AMP_HDLR_OK;
}

/**
  ******************************************************************************
  * @file           : I2cHdlr.h
  * @brief          : I2c Handler header
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

#ifndef I2C_HDLR_H
#define I2C_HDLR_H

typedef enum
{
	I2C_HDLR_OK = 0,
	I2C_HDLR_BUSY,
	I2C_HDLR_NODATA,
	I2C_HDLR_ERR
}I2cHdlrErrCode;

typedef enum
{
    I2C_HDLR_MOD1 = 0,
    I2C_HDLR_MOD2,
    I2C_HDLR_MOD3,
} tI2cHdlrModIdx;

void I2cHdlrInit(void);
void I2cHdlrRun(void);
I2cHdlrErrCode I2cHdlrMasterTx (tI2cHdlrModIdx devIdx, uint8_t addr, uint8_t *data, uint16_t length);
I2cHdlrErrCode I2cHdlrMasterRx (tI2cHdlrModIdx devIdx, uint8_t addr, uint8_t *data, uint16_t length);
I2cHdlrErrCode I2cHdlrTxRun (tI2cHdlrModIdx devIdx);
I2cHdlrErrCode I2cHdlrRxRun (tI2cHdlrModIdx devIdx);
boolean I2cHdlrIsFsmBusy (tI2cHdlrModIdx devIdx);
#endif

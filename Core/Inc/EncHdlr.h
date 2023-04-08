/**
  ******************************************************************************
  * @file           : EncHdlr.h
  * @brief          : Encoder Handler header
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


#ifndef ENC_HDLR_H
#define ENC_HDLR_H

typedef enum
{
	ENC_HDLR_OK = 0,
	ENC_HDLR_BUSY,
	ENC_HDLR_ERR
}EncHdlrErrCode;

EncHdlrErrCode EncHdlrInit(void);
EncHdlrErrCode EncHdlrRun(void);
#endif

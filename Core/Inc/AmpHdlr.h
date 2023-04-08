/**
  ******************************************************************************
  * @file           : AmpHdlr.h
  * @brief          : Amplifier Handler header
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

#ifndef AMP_HDLR_H
#define AMP_HDLR_H

typedef enum
{
	AMP_HDLR_OK = 0,
	AMP_HDLR_BUSY,
	AMP_HDLR_ERR
}AmpHdlrErrCode;

AmpHdlrErrCode AmpHdlrInit(void);
AmpHdlrErrCode AmpHdlrRun(void);
AmpHdlrErrCode AmpHdlrSetGain (uint8_t gain);
#endif

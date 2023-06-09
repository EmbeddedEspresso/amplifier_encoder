/**
  ******************************************************************************
  * @file           : DebugHdlr.c
  * @brief          : Debug Handler
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


typedef enum
{
    DEBUG_HDLR_INIT = 0,
    DEBUG_HDLR_IDLE,
    DEBUG_HDLR_PRINT_MENU,
    DEBUG_HDLR_PRINT_MENU_SMS,
    DEBUG_HDLR_READ_CHOICE,
    DEBUG_HDLR_READ_CHOICE_SMS,
    DEBUG_HDLR_GET_SIGNAL_WAIT,
    DEBUG_HDLR_GET_DATE_TIME_WAIT,
    DEBUG_HDLR_READ_SMS_WAIT,
    DEBUG_HDLR_SEND_SMS_WAIT,
    DEBUG_HDLR_DELETE_SMS_WAIT,
    DEBUG_HDLR_DIRECT_MODEM_DEBUG,
    DEBUG_HDLR_ERR_STS

}DebugHdlrFsmSts;

static DebugHdlrFsmSts fsmsts = DEBUG_HDLR_INIT;
static uint8_t isMenuActive = 0;
static uint8_t dRssi, dBer;
static uint8_t dYear, dMonth, dDay, dHour, dMin, dSec;
static uint8_t dDirectByte;
static char gMsg[256];
static char gNum[32];
static char debugLocalStr[256];
static char menuString[] = "\r\n\r\n1) Get modem signal level\r\n2) Get date time\r\n3) SMS handling\r\n4) Do a call\r\n5) Direct modem debug\r\n6) Restart Application\r\n7) Quit menu\r\n\r\n";

DebugHdlrErrCode DebugHdlrInit (void)
{
    fsmsts = DEBUG_HDLR_INIT;
}

DebugHdlrErrCode DebugHdlrRun (void)
{
    DebugHdlrErrCode result = DEBUG_HDLR_OK;
    static int tmr;
    uint32_t ansIdx;
    uint8_t readByte;

    switch (fsmsts)
    {
        case DEBUG_HDLR_INIT:
            PRINT_DEBUG("[Debug]: Initialization completed\r\n");
            fsmsts = DEBUG_HDLR_IDLE;
            break;

        case DEBUG_HDLR_IDLE:
            /* Check the input buffer */
            if (UartDebugHdlrRx(&readByte, 1) == COM_DEBUG_HDLR_OK)
            {
                if (readByte == 'm')
                {
                    isMenuActive = 1;
                    fsmsts = DEBUG_HDLR_PRINT_MENU;
                }
                UartDebugHdlrFlushRx();
            }

            break;

        case DEBUG_HDLR_PRINT_MENU:
            UartDebugHdlrTx(menuString, strlen(menuString));
            fsmsts = DEBUG_HDLR_READ_CHOICE;
            break;

        case DEBUG_HDLR_READ_CHOICE:
            if (UartDebugHdlrRx(&readByte, 1) == COM_DEBUG_HDLR_OK)
            {
                switch(readByte)
                {
                case '1':
                    if (1)
                    {
                        UartDebugHdlrTx("Command accepted\r\n", 18);
                        fsmsts = DEBUG_HDLR_GET_SIGNAL_WAIT;
                    }
                    else
                    {
                        UartDebugHdlrTx("Command rejected\r\n", 18);
                        fsmsts = DEBUG_HDLR_PRINT_MENU;
                    }
                    break;

                case '2':
                    if (1)
                    {
                        UartDebugHdlrTx("Command accepted\r\n", 18);
                        fsmsts = DEBUG_HDLR_GET_DATE_TIME_WAIT;
                    }
                    else
                    {
                        UartDebugHdlrTx("Command rejected\r\n", 18);
                        fsmsts = DEBUG_HDLR_PRINT_MENU;
                    }
                    break;

                case '3':
                    fsmsts = DEBUG_HDLR_PRINT_MENU_SMS;
                    break;

                case '5':
                    UartDebugHdlrTx("Direct modem mode started ('q' for quit)\r\n\r\n", 44);
                    fsmsts = DEBUG_HDLR_DIRECT_MODEM_DEBUG;
                    break;

                case '6':
                    NVIC_SystemReset();
                    break;

                case '7':
                    isMenuActive = 0;
                    fsmsts = DEBUG_HDLR_IDLE;
                    break;

                default:
                    fsmsts = DEBUG_HDLR_PRINT_MENU;
                    break;
                }
                UartDebugHdlrFlushRx();
            }

            break;


        case DEBUG_HDLR_ERR_STS:
            break;

    }

    return result;
}

DebugHdlrErrCode DebugHdlrPrintMsg(uint8_t *buff)
{
    if (isMenuActive == 0)
    {
        return UartDebugHdlrTx(buff, strlen(buff));
    }
}

// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#include "MessageParser.h"
#include <assert.h>
#include <string.h>
#include <applibs/log.h>

static uint8_t Buffer[256];
#define BUFFER_BEGIN    (Buffer)
#define BUFFER_END      (Buffer + sizeof(Buffer) / sizeof(Buffer[0]))

static uint8_t* ScannedSpanEnd = BUFFER_BEGIN;
static uint8_t* BeforeScanEnd = BUFFER_BEGIN;

BytesSpan_t MessageParserGetReceiveBuffer(void)
{
    return BytesSpanInit(BeforeScanEnd,BUFFER_END);
}

void MessageParserAddReceivedSize(size_t size)
{
    BeforeScanEnd += size;
    assert(BeforeScanEnd <= BUFFER_END);
}

void MessageParserDoWork(void)
{
    BytesSpan_t scanSpan;
    uint8_t* scanPtr;
    do {
        scanSpan = BytesSpanInit(ScannedSpanEnd, BeforeScanEnd);
        for (scanPtr = scanSpan.Begin; scanPtr != scanSpan.End; ++scanPtr) {
            if (*scanPtr == 0x0a) {
                BytesSpan_t messageSpan = BytesSpanInit(BUFFER_BEGIN, scanPtr);
                Log_Debug("[");
                for (uint8_t* p = messageSpan.Begin; p != messageSpan.End; ++p) Log_Debug("%c", *p);
                Log_Debug("]\n");

                const size_t beforeScanSize = (size_t)(scanSpan.End - (scanPtr + 1));
                memmove(BUFFER_BEGIN, scanPtr + 1, beforeScanSize);
                ScannedSpanEnd = BUFFER_BEGIN;
                BeforeScanEnd = BUFFER_BEGIN + beforeScanSize;
                break;
            }
        }
    } while (scanPtr != scanSpan.End);
    ScannedSpanEnd = BeforeScanEnd = scanSpan.End;
}

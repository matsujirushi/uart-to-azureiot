// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#pragma once

#include "BytesSpan.h"

void MessageParserSetMessageReceivedHandler(void(*handler)(BytesSpan_t messageSpan));
BytesSpan_t MessageParserGetReceiveBuffer(void);
void MessageParserAddReceivedSize(size_t size);
void MessageParserDoWork(void);

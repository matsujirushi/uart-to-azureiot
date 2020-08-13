// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t* Begin;
    uint8_t* End;
} BytesSpan_t;

BytesSpan_t BytesSpanInit(uint8_t* begin, uint8_t* end);
size_t BytesSpanSize(const BytesSpan_t* bytesSpan);

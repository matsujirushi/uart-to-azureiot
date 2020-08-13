// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#include "BytesSpan.h"

BytesSpan_t BytesSpanInit(uint8_t* begin, uint8_t* end)
{
    BytesSpan_t span = { .Begin = begin, .End = end };
    return span;
}

size_t BytesSpanSize(const BytesSpan_t* bytesSpan)
{
    return (size_t)(bytesSpan->End - bytesSpan->Begin);
}

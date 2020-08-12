// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#pragma once

#include <stdbool.h>

bool Exit_IsExit(void);
int Exit_GetExitCode(void);
void Exit_DoExit(int exitCode);
void Exit_DoExitWithLog(int exitCode, const char* fmt, ...);

// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#include "Exit.h"
#include <signal.h>
#include <stdarg.h>
#include <applibs/log.h>

static volatile bool Exited = false;
static volatile sig_atomic_t ExitCode = 0;

bool Exit_IsExit(void)
{
	return Exited;
}

int Exit_GetExitCode(void)
{
	return ExitCode;
}

void Exit_DoExit(int exitCode)
{
	ExitCode = exitCode;
	Exited = true;
}

void Exit_DoExitWithLog(int exitCode, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	Log_DebugVarArgs(fmt, ap);
	va_end(ap);

	Exit_DoExit(exitCode);
}

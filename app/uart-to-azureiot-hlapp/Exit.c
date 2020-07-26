#include "Exit.h"
#include <signal.h>

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

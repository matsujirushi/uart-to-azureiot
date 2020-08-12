// Copyright (c) 2020 matsujirushi
// Released under the MIT license
// https://github.com/matsujirushi/uart-to-azureiot/blob/master/LICENSE.txt

#include "Termination.h"
#include <signal.h>
#include <string.h>
#include "Exit.h"

static volatile sig_atomic_t ExitCode;

static void TerminationHandler(int signalNumber)
{
	Exit_DoExit(ExitCode);
}

void Termination_SetExitCode(int exitCode)
{
	ExitCode = exitCode;

	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = TerminationHandler;
	sigaction(SIGTERM, &action, NULL);
}

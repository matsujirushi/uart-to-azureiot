#pragma once

#include <stdbool.h>

bool Exit_IsExit(void);
int Exit_GetExitCode(void);
void Exit_DoExit(int exitCode);

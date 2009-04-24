#include <stdio.h>
#include <ogcsys.h>

#include "sys.h"
#include "wpad.h"


void Restart(void)
{
	printf("\n    Restarting Wii...");
	fflush(stdout);

	/* Load system menu */
	Sys_LoadMenu();
}

void Restart_Wait(void)
{
	printf("\n");

	printf("    Press any button to restart...");
	fflush(stdout);

	/* Wait for button */
	Wpad_WaitButtons();

	printf(" Restarting Wii...");
	fflush(stdout);

	/* Load system menu */
	Sys_LoadMenu();
}
 

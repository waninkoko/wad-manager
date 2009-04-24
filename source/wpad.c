#include <stdio.h>
#include <ogcsys.h>

#include "wpad.h"

/* Constants */
#define MAX_WIIMOTES	4

s32 wpad_init(void)
{
	/* Initialize Wiimote subsystem */
	return WPAD_Init();
}

u32 wpad_getbuttons(void)
{
	u32 buttons = 0, cnt;

	/* Scan pads */
	WPAD_ScanPads();

	/* Get pressed buttons */
	for (cnt = 0; cnt < MAX_WIIMOTES; cnt++)
		buttons |= WPAD_ButtonsDown(cnt);

	return buttons;
}

u32 wpad_waitbuttons(void)
{
	u32 buttons = 0;

	/* Wait for button pressing */
	while (!buttons) {
		buttons = wpad_getbuttons();
		VIDEO_WaitVSync();
	}

	return buttons;
}

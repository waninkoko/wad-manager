#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <sys/dir.h>

#include "fat.h"
#include "menu.h"
#include "restart.h"
#include "usbstorage.h"
#include "video.h"
#include "wad.h"
#include "wpad.h"

/* Device list */
static struct fatDevice deviceList[] = {
	{ "sd",		"Wii SD Slot",			&__io_wiisd },
	{ "usb",	"USB Mass Storage Device",	&__io_usbstorage },
	{ "usb2",	"USB 2.0 Mass Storage Device",	&__io_usb2storage },
	{ "gcsda",	"SD Gecko (Slot A)",		&__io_gcsda },
	{ "gcsdb",	"SD Gecko (Slot B)",		&__io_gcsdb },
};

/* Macros */
#define NB_DEVICES	(sizeof(deviceList) / sizeof(struct fatDevice))

/* Constants */
#define ENTRIES_PER_PAGE	10
#define WAD_DIRECTORY		"/wad"

/* Filelist variables */
static char fileList[65535] = { 0 };
static u32  fileCnt = 0;

/* Variables */
static s32 selected = 0, start = 0;


s32 __Menu_RetrieveList(void)
{
	char     *ptr = fileList;
	DIR_ITER *dir = NULL;

	struct stat filestat;

	/* Open directory */
	dir = diropen(WAD_DIRECTORY);
	if (!dir)
		return -1;

	/* Reset file counter */
	fileCnt = 0;

	/* Erase file list */
	memset(fileList, 0, sizeof(fileList));

	/* Get directory entries */
	while (!dirnext(dir, ptr, &filestat))
		if (!(filestat.st_mode & S_IFDIR)) {
			ptr += strlen(ptr) + 1;
			fileCnt++;
		}

	/* Close directory */
	dirclose(dir);

	return 0;
}

void __Menu_MoveList(s8 delta)
{
	s32 index;

	/* Select next entry */
	selected += delta;

	/* Out of the list? */
	if (selected <= -1)
		selected = (fileCnt- 1);
	if (selected >= fileCnt)
		selected = 0;

	/* List scrolling */
	index = (selected - start);

	if (index >= ENTRIES_PER_PAGE)
		start += index - (ENTRIES_PER_PAGE - 1);
	if (index <= -1)
		start += index;
}

void __Menu_PrintList(void)
{
	u32 cnt, idx;

	printf("[+] Available WAD files on the device:\n\n");

	/* No files */
	if (!fileCnt) {
		printf("\t\t>> No files found!\n");
		return;
	}

	/* Move to start entry */
	for (cnt = idx = 0; cnt < start; cnt++)
		idx += strlen(fileList + idx) + 1;

	/* Print entries */
	for (cnt = start; cnt < fileCnt; cnt++) {
		/* Entries per page limit */
		if ((cnt - start) >= ENTRIES_PER_PAGE)
			break;

		/* Print filename */
		printf("\t\t%2s %s\n", (cnt == selected) ? ">>" : "  ", fileList + idx);

		/* Move to the next entry */
		idx += strlen(fileList + idx) + 1;
	}
}

void __Menu_Controls(void)
{
	u32 buttons = Wpad_WaitButtons();

	/* UP/DOWN buttons */
	if (buttons & WPAD_BUTTON_UP)
		__Menu_MoveList(-1);
	if (buttons & WPAD_BUTTON_DOWN)
		__Menu_MoveList(1);

	/* LEFT/RIGHT buttons */
	if (buttons & WPAD_BUTTON_LEFT)
		__Menu_MoveList(-ENTRIES_PER_PAGE);
	if (buttons & WPAD_BUTTON_RIGHT)
		__Menu_MoveList(ENTRIES_PER_PAGE);


	/* HOME button */
	if (buttons & WPAD_BUTTON_HOME)
		Restart();

	/* PLUS (+) button */
	if (buttons & WPAD_BUTTON_PLUS)
		Menu_Manage(selected, 'i');

	/* MINUS (-) button */
	if (buttons & WPAD_BUTTON_MINUS)
		Menu_Manage(selected, 'u');

	/* ONE (1) button */
	if (buttons & WPAD_BUTTON_1)
		Menu_Device();
}


void Menu_Device(void)
{
	struct fatDevice *device = NULL;

	s32 ret, selected = 0;

loop:
	/* Select source device */
	for (;;) {
		/* Clear console */
		Con_Clear();

		/* Selected device */
		device = &deviceList[selected];

		printf("\t>> Select source device: < %s >\n\n", device->name);

		printf("\t   Press LEFT/RIGHT to change the selected device.\n");
		printf("\t   Press A to continue.\n\n");

		u32 buttons = Wpad_WaitButtons();

		/* LEFT/RIGHT buttons */
		if (buttons & WPAD_BUTTON_LEFT) {
			if ((--selected) <= -1)
				selected = (NB_DEVICES - 1);
		}
		if (buttons & WPAD_BUTTON_RIGHT) {
			if ((++selected) >= NB_DEVICES)
				selected = 0;
		}

		/* HOME button */
		if (buttons & WPAD_BUTTON_HOME)
			Restart();

		/* A button */
		if (buttons & WPAD_BUTTON_A)
			break;
	}

	/* Mount device */
	printf("[+] Mounting device, please wait...");
	fflush(stdout);

	/* Mount device */
	ret = Fat_Mount(device);
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto err;
	} else
		printf(" OK!\n");

	printf("[+] Retrieving file list...");
	fflush(stdout);

	ret = __Menu_RetrieveList();
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto err;
	} else
		printf(" OK!\n");

	return;

err:
	printf("\n");
	printf("    Press any button to continue...\n");

	Wpad_WaitButtons();

	/* Prompt menu again */
	goto loop;
}

void Menu_Manage(u32 index, u8 mode)
{
	char filepath[128];

	char *ptr = fileList;
	FILE *fp  = NULL;

	u32 cnt;

	/* Move to the specified entry */
	for (cnt = 0; cnt < index; cnt++)
		ptr += strlen(ptr) + 1;

	/* Ask user */
	switch (mode) {
	case 'i':
		printf("[+] Are you sure you want to install this WAD file?\n\n");
		break;

	case 'u':
		printf("[+] Are you sure you want to uninstall this WAD file?\n\n");
		break;
	}

	printf("    Filename: %s\n\n", ptr);

	printf("    Press A to continue.\n");
	printf("    Press B to return to the menu.\n\n");

	/* Wait for user answer */
	for (;;) {
		u32 buttons = Wpad_WaitButtons();

		/* A button */
		if (buttons & WPAD_BUTTON_A)
			break;

		/* B button */
		if (buttons & WPAD_BUTTON_B)
			return;
	}

	/* Generate filepath */
	sprintf(filepath, WAD_DIRECTORY "/%s", ptr);

	printf("[+] Opening WAD file, please wait...");
	fflush(stdout);

	/* Open WAD */
	fp = fopen(filepath, "rb");
	if (!fp) {
		printf(" ERROR!\n");
		goto out;
	} else
		printf(" OK!\n\n");

	/* (Un)Install WAD */
	switch (mode) {
	case 'i':
		printf("[+] Installing WAD, please wait...\n");

		/* Install WAD */
		Wad_Install(fp);

		break;
	case 'u':
		printf("[+] Uninstalling WAD, please wait...\n");

		/* Uninstall WAD */
		Wad_Uninstall(fp);

		break;
	}

out:
	/* Close file */
	if (fp)
		fclose(fp);

	printf("\n");
	printf("    Press any button to continue...\n");

	/* Wait for button */
	Wpad_WaitButtons();
}

void Menu_Loop(void)
{
	/* Device menu */
	Menu_Device();

	/* Menu loop */
	for (;;) {
		/* Clear console */
		Con_Clear();

		/* Print entries */
		__Menu_PrintList();

		/* Controls */
		__Menu_Controls();
	}
}

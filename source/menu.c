#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>

#include "fat.h"
#include "menu.h"
#include "restart.h"
#include "usbstorage.h"
#include "utils.h"
#include "video.h"
#include "wad.h"
#include "wpad.h"

/* Device list variables */
static fatDevice deviceList[] = {
	{ "sd",		"Wii SD Slot",			&__io_wiisd },
	{ "usb",	"USB Mass Storage Device",	&__io_usbstorage },
	{ "usb2",	"USB 2.0 Mass Storage Device",	&__io_usb2storage },
	{ "gcsda",	"SD Gecko (Slot A)",		&__io_gcsda },
	{ "gcsdb",	"SD Gecko (Slot B)",		&__io_gcsdb },
};

static s32 device = 0;

/* File list variables */
static fatFile *fileList = NULL;
static u32      fileCnt  = 0;

/* Variables */
static s32 selected = 0, start = 0;

/* Macros */
#define NB_DEVICES		(sizeof(deviceList) / sizeof(fatDevice))

/* Constants */
#define ENTRIES_PER_PAGE	10
#define WAD_DIRECTORY		"/wad"


s32 __Menu_EntryCmp(const void *p1, const void *p2)
{
	fatFile *f1 = (fatFile *)p1;
	fatFile *f2 = (fatFile *)p2;

	/* Compare entries */
	return strcmp(f1->filename, f2->filename);
}

s32 __Menu_RetrieveList(void)
{
	fatDevice *dev = &deviceList[device];
	DIR_ITER  *dir = NULL;

	struct stat filestat;

	char dirpath[128], filename[1024];
	u32  cnt = 0, len;

	/* Generate dirpath */
	sprintf(dirpath, "%s:" WAD_DIRECTORY, dev->mount);

	/* Open directory */
	dir = diropen(dirpath);
	if (!dir)
		return -1;

	/* Count entries */
	while (!dirnext(dir, filename, &filestat)) {
		if (!(filestat.st_mode & S_IFDIR))
			cnt++;
	}

	/* Reset directory */
	dirreset(dir);

	/* Buffer length */
	len = sizeof(fatFile) * cnt;

	/* Allocate memory */
	fileList = (fatFile *)realloc(fileList, len);
	if (!fileList) {
		dirclose(dir);
		return -2;
	}

	/* Reset file counter */
	fileCnt = 0;

	/* Get entries */
	while (!dirnext(dir, filename, &filestat)) {
		if (!(filestat.st_mode & S_IFDIR)) {
			fatFile *file = &fileList[fileCnt++];

			/* Copy file info */
			strcpy(file->filename, filename);
			file->filestat = filestat;
		}
	}

	/* Close directory */
	dirclose(dir);

	/* Sort list */
	qsort(fileList, fileCnt, sizeof(fatFile), __Menu_EntryCmp);

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
	u32 cnt;

	printf("[+] Available WAD files on the device:\n\n");

	/* No files */
	if (!fileCnt) {
		printf("\t\t>> No files found!\n");
		return;
	}

	/* Print entries */
	for (cnt = start; cnt < fileCnt; cnt++) {
		fatFile *file = &fileList[cnt];

		/* File size in megabytes */
		f32 filesize = file->filestat.st_size / MB_SIZE;

		/* Entries per page limit */
		if ((cnt - start) >= ENTRIES_PER_PAGE)
			break;

		/* Print filename */
		printf("\t\t%2s %s (%.2f MB)\n", (cnt == selected) ? ">>" : "  ", file->filename, filesize);
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
	fatDevice *dev = NULL;

	s32 ret;

	/* Select source device */
	for (;;) {
		/* Clear console */
		Con_Clear();

		/* Selected device */
		dev = &deviceList[device];

		printf("\t>> Select source device: < %s >\n\n", dev->name);

		printf("\t   Press LEFT/RIGHT to change the selected device.\n\n");

		printf("\t   Press A button to continue.\n");
		printf("\t   Press HOME button to restart.\n\n\n");

		u32 buttons = Wpad_WaitButtons();

		/* LEFT/RIGHT buttons */
		if (buttons & WPAD_BUTTON_LEFT) {
			if ((--device) <= -1)
				device = (NB_DEVICES - 1);
		}
		if (buttons & WPAD_BUTTON_RIGHT) {
			if ((++device) >= NB_DEVICES)
				device = 0;
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
	ret = Fat_Mount(dev);
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto err;
	} else
		printf(" OK!\n");

	printf("[+] Retrieving file list...");
	fflush(stdout);

	/* Retrieve filelist */
	ret = __Menu_RetrieveList();
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto err;
	} else
		printf(" OK!\n");

	return;

err:
	/* Unmount device */
	Fat_Unmount(dev);

	printf("\n");
	printf("    Press any button to continue...\n");

	Wpad_WaitButtons();

	/* Prompt menu again */
	Menu_Device();
}

void Menu_Manage(u32 index, u8 mode)
{
	fatDevice *dev  = NULL;
	fatFile   *file = NULL;

	FILE *fp = NULL;

	char filepath[128];
	f32  filesize;

	/* No files */
	if (!fileCnt)
		return;

	/* Selected file/device */
	dev  = &deviceList[device];
	file = &fileList[index];

	/* File size in megabytes */
	filesize = (file->filestat.st_size / MB_SIZE);

	/* Clear console */
	Con_Clear();

	/* Ask user */
	switch (mode) {
	case 'i':
		printf("[+] Are you sure you want to install this WAD file?\n\n");
		break;

	case 'u':
		printf("[+] Are you sure you want to uninstall this WAD file?\n\n");
		break;
	}

	printf("    Filename: %s\n",        file->filename);
	printf("    Filesize: %.2f MB\n\n", filesize);

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
	sprintf(filepath, "%s:" WAD_DIRECTORY "/%s", dev->mount, file->filename);

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

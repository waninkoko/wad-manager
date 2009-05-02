#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>

#include "fat.h"
#include "restart.h"
#include "title.h"
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


/* Variables */
static s32 device = 0;

/* Macros */
#define NB_DEVICES		(sizeof(deviceList) / sizeof(fatDevice))

/* Constants */
#define CIOS_VERSION		249
#define ENTRIES_PER_PAGE	8

#define WAD_DIRECTORY		"/wad"


s32 __Menu_IsGreater(const void *p1, const void *p2)
{
	u32 n1 = *(u32 *)p1;
	u32 n2 = *(u32 *)p2;

	/* Equal */
	if (n1 == n2)
		return 0;

	return (n1 > n2) ? 1 : -1;
}

s32 __Menu_EntryCmp(const void *p1, const void *p2)
{
	fatFile *f1 = (fatFile *)p1;
	fatFile *f2 = (fatFile *)p2;

	/* Compare entries */
	return strcmp(f1->filename, f2->filename);
}

s32 __Menu_RetrieveList(fatFile **outbuf, u32 *outlen)
{
	fatFile  *buffer = NULL;
	DIR_ITER *dir    = NULL;

	struct stat filestat;

	char dirpath[128], filename[1024];
	u32  cnt;

	/* Generate dirpath */
	sprintf(dirpath, "%s:" WAD_DIRECTORY, deviceList[device].mount);

	/* Open directory */
	dir = diropen(dirpath);
	if (!dir)
		return -1;

	/* Count entries */
	for (cnt = 0; !dirnext(dir, filename, &filestat);) {
		if (!(filestat.st_mode & S_IFDIR))
			cnt++;
	}

	if (cnt > 0) {
		/* Allocate memory */
		buffer = malloc(sizeof(fatFile) * cnt);
		if (!buffer) {
			dirclose(dir);
			return -2;
		}

		/* Reset directory */
		dirreset(dir);

		/* Get entries */
		for (cnt = 0; !dirnext(dir, filename, &filestat);) {
			if (!(filestat.st_mode & S_IFDIR)) {
				fatFile *file = &buffer[cnt++];

				/* File name */
				strcpy(file->filename, filename);

				/* File stats */
				file->filestat = filestat;
			}
		}

		/* Sort list */
		qsort(buffer, cnt, sizeof(fatFile), __Menu_EntryCmp);
	}

	/* Close directory */
	dirclose(dir);

	/* Set values */
	*outbuf = buffer;
	*outlen = cnt;

	return 0;
}


void Menu_SelectIOS(void)
{
	u8 *iosVersion = NULL;
	u32 iosCnt;

	u32 cnt;
	s32 ret, selected = 0;

	/* Get IOS versions */
	ret = Title_GetIOSVersions(&iosVersion, &iosCnt);
	if (ret < 0)
		return;

	/* Sort list */
	qsort(iosVersion, iosCnt, sizeof(u8), __Menu_IsGreater);

	/* Set default version */
	for (cnt = 0; cnt < iosCnt; cnt++) {
		u8 version = iosVersion[cnt];

		/* Custom IOS available */
		if (version == CIOS_VERSION) {
			selected = cnt;
			break;
		}

		/* Current IOS */
		if (version == IOS_GetVersion())
			selected = cnt;
	}

	/* Ask user for IOS version */
	for (;;) {
		/* Clear console */
		Con_Clear();

		printf("\t>> Select IOS version to use: < IOS%d >\n\n", iosVersion[selected]);

		printf("\t   Press LEFT/RIGHT to change IOS version.\n\n");

		printf("\t   Press A button to continue.\n");
		printf("\t   Press HOME button to restart.\n\n");

		u32 buttons = Wpad_WaitButtons();

		/* LEFT/RIGHT buttons */
		if (buttons & WPAD_BUTTON_LEFT) {
			if ((--selected) <= -1)
				selected = (iosCnt - 1);
		}
		if (buttons & WPAD_BUTTON_RIGHT) {
			if ((++selected) >= iosCnt)
				selected = 0;
		}

		/* HOME button */
		if (buttons & WPAD_BUTTON_HOME)
			Restart();

		/* A button */
		if (buttons & WPAD_BUTTON_A)
			break;
	}


	u8 version = iosVersion[selected];

	if (IOS_GetVersion() != version) {
		/* Shutdown subsystems */
		Wpad_Disconnect();

		/* Load IOS */
		ret = IOS_ReloadIOS(version);

		/* Initialize subsystems */
		Wpad_Init();
	}
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
		printf("\t   Press HOME button to restart.\n\n");

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

void Menu_WadManage(fatFile *file)
{
	fatDevice *dev = &deviceList[device];
	FILE      *fp  = NULL;

	char filepath[128];
	f32  filesize;

	u32  mode = 0;

	/* File size in megabytes */
	filesize = (file->filestat.st_size / MB_SIZE);

	for (;;) {
		/* Clear console */
		Con_Clear();

		printf("[+] WAD Filename : %s\n",          file->filename);
		printf("    WAD Filesize : %.2f MB\n\n\n", filesize);


		printf("[+] Select action: < %s WAD >\n\n",  (!mode) ? "Install" : "Uninstall");

		printf("    Press LEFT/RIGHT to change selected action.\n\n");

		printf("    Press A to continue.\n");
		printf("    Press B to go back to the menu.\n\n");

		u32 buttons = Wpad_WaitButtons();

		/* LEFT/RIGHT buttons */
		if (buttons & (WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT))
			mode ^= 1;

		/* A button */
		if (buttons & WPAD_BUTTON_A)
			break;

		/* B button */
		if (buttons & WPAD_BUTTON_B)
			return;
	}

	/* Clear console */
	Con_Clear();

	printf("[+] Opening \"%s\", please wait...", file->filename);
	fflush(stdout);

	/* Generate filepath */
	sprintf(filepath, "%s:" WAD_DIRECTORY "/%s", dev->mount, file->filename);

	/* Open WAD */
	fp = fopen(filepath, "rb");
	if (!fp) {
		printf(" ERROR!\n");
		goto out;
	} else
		printf(" OK!\n\n");

	printf("[+] %s WAD, please wait...\n", (!mode) ? "Installing" : "Uninstalling");

	/* Do install/uninstall */
	if (!mode)
		Wad_Install(fp);
	else
		Wad_Uninstall(fp);

out:
	/* Close file */
	if (fp)
		fclose(fp);

	printf("\n");
	printf("    Press any button to continue...\n");

	/* Wait for button */
	Wpad_WaitButtons();
}

void Menu_WadList(void)
{
	fatFile *fileList = NULL;
	u32      fileCnt;

	s32 ret, selected = 0, start = 0;

	printf("[+] Retrieving file list...");
	fflush(stdout);

	/* Retrieve filelist */
	ret = __Menu_RetrieveList(&fileList, &fileCnt);
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto err;
	}

	/* No files */
	if (!fileCnt) {
		printf(" No files found!\n");
		goto err;
	}

	for (;;) {
		u32 cnt;
		s32 index;

		/* Clear console */
		Con_Clear();

		/** Print entries **/
		printf("[+] Available WAD files on the device:\n\n");

		/* Print entries */
		for (cnt = start; cnt < fileCnt; cnt++) {
			fatFile *file     = &fileList[cnt];
			f32      filesize = file->filestat.st_size / MB_SIZE;

			/* Entries per page limit */
			if ((cnt - start) >= ENTRIES_PER_PAGE)
				break;

			/* Print filename */
			printf("\t%2s %s (%.2f MB)\n", (cnt == selected) ? ">>" : "  ", file->filename, filesize);
		}

		printf("\n");

		printf("[+] Press A button to (un)install a WAD file.\n");
		printf("    Press B button to select a storage device.\n");

		/** Controls **/
		u32 buttons = Wpad_WaitButtons();

		/* DPAD buttons */
		if (buttons & (WPAD_BUTTON_UP | WPAD_BUTTON_LEFT)) {
			selected -= (buttons & WPAD_BUTTON_LEFT) ? ENTRIES_PER_PAGE : 1;

			if (selected <= -1)
				selected = (fileCnt - 1);
		}
		if (buttons & (WPAD_BUTTON_DOWN | WPAD_BUTTON_RIGHT)) {
			selected += (buttons & WPAD_BUTTON_RIGHT) ? ENTRIES_PER_PAGE : 1;

			if (selected >= fileCnt)
				selected = 0;
		}

		/* HOME button */
		if (buttons & WPAD_BUTTON_HOME)
			Restart();

		/* A button */
		if (buttons & WPAD_BUTTON_A)
			Menu_WadManage(&fileList[selected]);

		/* B button */
		if (buttons & WPAD_BUTTON_B)
			return;

		/** Scrolling **/
		/* List scrolling */
		index = (selected - start);

		if (index >= ENTRIES_PER_PAGE)
			start += index - (ENTRIES_PER_PAGE - 1);
		if (index <= -1)
			start += index;
	}

err:
	printf("\n");
	printf("    Press any button to continue...\n");

	/* Wait for button */
	Wpad_WaitButtons();
}


void Menu_Loop(void)
{
	/* Select IOS menu */
	Menu_SelectIOS();

	for (;;) {
		/* Device menu */
		Menu_Device();

		/* WAD list menu */
		Menu_WadList();
	}
}

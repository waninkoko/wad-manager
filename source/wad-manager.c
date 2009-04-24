#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <fat.h>
#include <sys/dir.h>

#include "sys.h"
#include "video.h"
#include "wad.h"
#include "wpad.h"

/* libfat constants */
#define DEVICE_MIN		PI_SDGECKO_A
#define DEVICE_MAX		PI_USBSTORAGE

/* filelist constants */
#define FILES_PER_PAGE		10
#define WAD_DIRECTORY		"wad"

/* console constants */
#define CONSOLE_XCOORD		20
#define CONSOLE_YCOORD		100

/* Constants */
#define MAX_FILELIST_LEN	65536
#define MAX_FILEPATH_LEN	256

/* Variables */
static char filelist[MAX_FILELIST_LEN];
static s32  device = PI_INTERNAL_SD, nb_files = 0, selected = 0, start = 0;


void restart(void)
{
	printf("\n    Restarting Wii...");
	fflush(stdout);

	/* Load system menu */
	sys_loadmenu();
}

void restart_wait(void)
{
	printf("\n    Press any button to restart...");
	fflush(stdout);

	/* Wait for button */
	wpad_waitbuttons();

	printf(" Restarting Wii...");
	fflush(stdout);

	/* Load system menu */
	sys_loadmenu();
}

void show_banner(void)
{
	extern char banner_data[];

	PNGUPROP imgProp;
	IMGCTX ctx;

	s32 ret;

	/* Select PNG data */
	ctx = PNGU_SelectImageFromBuffer(banner_data);
	if (!ctx)
		return;

	/* Get image properties */
	ret = PNGU_GetImageProperties(ctx, &imgProp);
	if (ret != PNGU_OK)
		return;

	/* Draw image */
	video_drawpng(ctx, imgProp, 0, 0);

	/* Free image context */
	PNGU_ReleaseImageContext(ctx);
}

void print_disclaimer(void)
{
	/* Print disclaimer */
	printf("[+] [DISCLAIMER]:\n\n");

	printf("    THIS APPLICATION COMES WITH NO WARRANTY AT ALL,\n");
	printf("    NEITHER EXPRESS NOR IMPLIED.\n");
	printf("    I DO NOT TAKE ANY RESPONSIBILITY FOR ANY DAMAGE IN YOUR WII CONSOLE\n");
	printf("    BECAUSE OF A IMPROPER USAGE OF THIS SOFTWARE.\n\n");
}

void filelist_scroll(s8 delta)
{
	s32 index;

	/* No files */
	if (!nb_files)
		return;

	/* Select next entry */
	selected += delta;

	/* Out of the list? */
	if (selected <= -1)
		selected = nb_files - 1;
	if (selected >= nb_files)
		selected = 0;

	/* List scrolling */
	index = (selected - start);

	if (index >= FILES_PER_PAGE)
		start += index - (FILES_PER_PAGE - 1);
	if (index <= -1)
		start += index;
}

void filelist_print(void)
{
	u32 cnt, idx = 0;

	printf("[+] Available files on the device:\n\n");

	/* No files */
	if (!nb_files) {
		printf("\t\t>> No files found!\n");
		return;
	}

	/* Move to start entry */
	for (cnt = 0; cnt < start; cnt++)
		idx += strlen(filelist + idx) + 1;

	/* Show entries */
	for (cnt = start; cnt < nb_files; cnt++) {
		/* Files per page limit */
		if ((cnt - start) >= FILES_PER_PAGE)
			break;

		/* Selected file */
		(cnt == selected) ? printf("\t\t>> ") : printf("\t\t   ");
		fflush(stdout);

		/* Print filename */
		printf("%s\n", filelist + idx);

		/* Move to the next entry */
		idx += strlen(filelist + idx) + 1;
	}
}

s32 filelist_retrieve(void)
{
	char dirpath[MAX_FILEPATH_LEN];
	char *ptr = filelist;

	struct stat filestat;
	DIR_ITER *dir;

	/* Generate dirpath */
	sprintf(dirpath, "fat%d:/" WAD_DIRECTORY, device);

	/* Open WAD directory */
	dir = diropen(dirpath);
	if (!dir)
		return -1;

	/* Reset file counter */
	nb_files = 0;

	/* Erase file list */
	memset(filelist, 0, sizeof(filelist));

	/* Get directory entries */
	while (!dirnext(dir, ptr, &filestat))
		if (!(filestat.st_mode & S_IFDIR)) {
			ptr += strlen(ptr) + 1;
			nb_files++;
		}

	/* Close directory */
	dirclose(dir);

	return 0;
}

void wad_manage(u32 index, u8 mode)
{
	char filepath[MAX_FILEPATH_LEN], *ptr = filelist;

	FILE *fp = NULL;
	u32  cnt;

	/* Move to the specified entry */
	for (cnt = 0; cnt < index; cnt++)
		ptr += strlen(ptr) + 1;

	printf("\n\n");

	/* Ask user */
	switch (mode) {
	case 'i':
		printf("[+] Are you sure you want to install \"%s\"?\n", ptr);
		break;
	case 'u':
		printf("[+] Are you sure you want to uninstall \"%s\"?\n", ptr);
		break;
	}
	
	printf("    Press A to continue or B to cancel...\n");

	/* Wait for user answer */
	for (;;) {
		u32 buttons = wpad_waitbuttons();

		/* Buttons pressed */
		if (buttons & WPAD_BUTTON_A)
			break;
		if (buttons & WPAD_BUTTON_B)
			return;
	}

	/* Generate filepath */
	sprintf(filepath, "fat%d:/" WAD_DIRECTORY "/%s", device, ptr);

	/* Clear screen */
	con_clear();

	printf("\n[+] Opening WAD file, please wait...");
	fflush(stdout);

	/* Open WAD */
	fp = fopen(filepath, "rb");
	if (!fp) {
		printf(" ERROR!\n");
		goto out;
	}
	printf(" OK!\n\n");

	/* (Un)Install WAD */
	switch (mode) {
	case 'i':
		printf("[+] Installing WAD, please wait...\n");

		/* Install WAD */
		WAD_InstallFile(fp);

		break;
	case 'u':
		printf("[+] Uninstalling WAD, please wait...\n");

		/* Uninstall WAD */
		WAD_UninstallFile(fp);

		break;
	}

out:
	/* Close file */
	if (fp)
		fclose(fp);

	printf("\n    Press any button to continue...\n");

	/* Wait for button */
	wpad_waitbuttons();
}

void controls(void)
{
	u32 buttons = wpad_waitbuttons();

	/* UP button */
	if (buttons & WPAD_BUTTON_UP)
		filelist_scroll(-1);

	/* DOWN button */
	if (buttons & WPAD_BUTTON_DOWN)
		filelist_scroll(1);

	/* HOME button */
	if (buttons & WPAD_BUTTON_HOME)
		restart();

	/* + button */
	if (buttons & WPAD_BUTTON_PLUS)
		wad_manage(selected, 'i');

	/* - button */
	if (buttons & WPAD_BUTTON_MINUS)
		wad_manage(selected, 'u');
}

int main(int argc, char **argv)
{
	s32 cios, ret;

	/* Initialize subsystems */
	sys_init();

	/* Load Custom IOS */
	cios = IOS_ReloadIOS(249);

	/* Set video mode */
	video_setmode();

	/* Draw banner */
	show_banner();

	/* Initialize console */
	con_init(CONSOLE_XCOORD, CONSOLE_YCOORD);

	/* Initialize Wiimote */
	wpad_init();

	if (cios >= 0)
		printf("[+] Custom IOS detected and loaded!\n\n\n");

	/* Print disclaimer */
	print_disclaimer();

	printf(">>  If you agree, press A button to continue.\n");
	printf(">>  Otherwise, press B button to restart your Wii.\n\n");

	/* Wait for user answer */
	for (;;) {
		u32 buttons = wpad_waitbuttons();

		if (buttons & WPAD_BUTTON_A)
			break;

		if (buttons & WPAD_BUTTON_B)
			restart();
	}

	/* Clear console */
	con_clear();

	/* Select source device */
	for (;;) {
		char *devname;

		/* Set device name */
		switch (device) {
		case PI_SDGECKO_A:
			devname = "SD Gecko (Slot A)";
			break;

		case PI_SDGECKO_B:
			devname = "SD Gecko (Slot B)";
			break;

		case PI_INTERNAL_SD:
			devname = "Wii SD Slot";
			break;

		case PI_USBSTORAGE:
			devname = "USB Mass Storage Device";
			break;

		default:
			devname = "Unknown!";
		}

		con_clearline();

		printf("\r\t\t>> Select source device: %s", devname);
		fflush(stdout);

		u32 buttons = wpad_waitbuttons();

		if (buttons & WPAD_BUTTON_LEFT) {
			if ((--device) < DEVICE_MIN)
				device = DEVICE_MAX;
		}

		if (buttons & WPAD_BUTTON_RIGHT) {
			if ((++device) > DEVICE_MAX)
				device = DEVICE_MIN;
		}

		if (buttons & WPAD_BUTTON_A)
			break;
	}

	printf("\n\n");

	printf("[+] Initializing libfat, please wait...");
	fflush(stdout);

	/* Initialize libfat */
	ret = fatInitDefault();
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto err;
	}
	printf(" OK!\n");


	printf("[+] Retrieving device contents, please wait...");
	fflush(stdout);

	/* Retrieve filelist */
	ret = filelist_retrieve();
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto err;
	}

loop:
	/* Clear screen */
	con_clear();

	/* Print filelist */
	filelist_print();

	/* Controls */
	controls();

	goto loop;

err:
	/* Restart Wii */
	restart_wait();

	return 0;
}

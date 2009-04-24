#include <stdio.h>
#include <ogcsys.h>

#include "sys.h"
#include "video.h"

/* Video variables */
static void *framebuffer = NULL;
static GXRModeObj *vmode = NULL;

void con_init(u32 x, u32 y)
{
	u32 w, h;

	/* Set console width and height */
	w = vmode->fbWidth - (x * 2);
	h = vmode->xfbHeight - (y + 20);

	/* Create console in the framebuffer */
	CON_InitEx(vmode, x, y, w, h);
}

void con_clear(void)
{
	/* Clear console */
	printf("\x1b[2J");
	fflush(stdout);
}

void con_clearline(void)
{
	s32 cols, rows;
	u32 cnt;

	printf("\r");
	fflush(stdout);

	/* Save cursor position */
	printf("\x1b[s");
	fflush(stdout);

	/* Get console metrics */
	CON_GetMetrics(&cols, &rows);

	/* Erase line */
	for (cnt = 0; cnt < cols; cnt++) {
		printf(" ");
		fflush(stdout);
	}

	/* Load cursor position */
	printf("\x1b[u");
	fflush(stdout);
}

void con_fgcolor(u32 color, u8 bold)
{
	/* Set foreground color */
	printf("\x1b[%u;%um", color + 30, bold);
	fflush(stdout);
}

void con_bgcolor(u32 color, u8 bold)
{
	/* Set background color */
	printf("\x1b[%u;%um", color + 40, bold);
	fflush(stdout);
}

void con_fillrow(u32 row, u32 color, u8 bold)
{
	s32 cols, rows;
	u32 cnt;

	/* Set color */
	printf("\x1b[%u;%um", color + 40, bold);
	fflush(stdout);

	/* Get console metrics */
	CON_GetMetrics(&cols, &rows);

	/* Save current row and col */
	printf("\x1b[s");
	fflush(stdout);

	/* Move to specified row */
	printf("\x1b[%u;0H", row);
	fflush(stdout);

	/* Fill row */
	for (cnt = 0; cnt < cols; cnt++) {
		printf(" ");
		fflush(stdout);
	}

	/* Load saved row and col */
	printf("\x1b[u");
	fflush(stdout);

	/* Set default color */
	con_bgcolor(0, 0);
	con_fgcolor(7, 1);
}

void video_setmode(void)
{
	/* Select preferred video mode */
	vmode = VIDEO_GetPreferredMode(NULL);

	/* Allocate memory for the framebuffer */
	framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));

	/* Configure the video subsystem */
	VIDEO_Configure(vmode);

	/* Setup video */
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if (vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	/* Clear the screen */
	VIDEO_ClearFrameBuffer(vmode, framebuffer, COLOR_BLACK);
}

void video_clear(s32 color)
{
	VIDEO_ClearFrameBuffer(vmode, framebuffer, color);
}

void video_drawpng(IMGCTX ctx, PNGUPROP imgProp, u16 x, u16 y)
{
	PNGU_DECODE_TO_COORDS_YCbYCr(ctx, x, y, imgProp.imgWidth, imgProp.imgHeight, vmode->fbWidth, vmode->xfbHeight, framebuffer);
}

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include "libpng/pngu/pngu.h"

/* Prototypes */
void Con_Init(u32, u32, u32, u32);
void Con_Clear(void);
void Con_ClearLine(void);
void Con_FgColor(u32, u8);
void Con_BgColor(u32, u8);
void Con_FillRow(u32, u32, u8);

void Video_Configure(GXRModeObj *);
void Video_SetMode(void);
void Video_Clear(s32);
void Video_DrawPng(IMGCTX, PNGUPROP, u16, u16);

#endif

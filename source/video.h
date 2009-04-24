#ifndef _VIDEO_H_
#define _VIDEO_H_

#include "libpng/pngu/pngu.h"

/* Prototypes */
void con_init(u32, u32);
void con_clear(void);
void con_clearline(void);
void con_fgcolor(u32, u8);
void con_bgcolor(u32, u8);
void con_fillrow(u32, u32, u8);

void video_setmode(void);
void video_clear(s32);
void video_drawpng(IMGCTX, PNGUPROP, u16, u16);

#endif

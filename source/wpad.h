#ifndef _WPAD_H_
#define _WPAD_H_

#include <wiiuse/wpad.h>

/* Prototypes */
s32 wpad_init(void);
u32 wpad_getbuttons(void);
u32 wpad_waitbuttons(void);

#endif

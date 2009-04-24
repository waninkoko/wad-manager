#ifndef _WPAD_H_
#define _WPAD_H_

#include <wiiuse/wpad.h>

/* Prototypes */
s32  Wpad_Init(void);
void Wpad_Disconnect(void);
u32  Wpad_GetButtons(void);
u32  Wpad_WaitButtons(void);

#endif

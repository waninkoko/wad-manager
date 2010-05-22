#include <ogcsys.h>
#include <asndlib.h>

#include "oggplayer.h"
#include "sound.h"

/* Externs */
extern u8  bgMusic[];
extern u32 bgMusic_Len;


void Sound_Init(void)
{
	/* Init ASND */
	ASND_Init();
} 

s32 Sound_Play(void)
{
	/* Play background music */
	return PlayOgg(bgMusic, bgMusic_Len, 0, OGG_INFINITE_TIME);
}

void Sound_Stop(void)
{
	/* Stop background music */
	if (Sound_IsPlaying())
		StopOgg();
}

s32 Sound_IsPlaying(void)
{
	/* Check if background music is playing */
	return (StatusOgg() == OGG_STATUS_RUNNING);
}

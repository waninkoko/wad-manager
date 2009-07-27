#include <stdio.h>
#include <ogcsys.h>

#include "nand.h"

/* Buffer */
static u32 inbuf[8] ATTRIBUTE_ALIGN(32);


s32 Nand_Mount(nandDevice *dev)
{
	s32 fd, ret;

	/* Open FAT module */
	fd = IOS_Open("fat", 0);
	if (fd < 0)
		return fd;

	/* Mount device */
	ret = IOS_Ioctlv(fd, dev->mountCmd, 0, 0, NULL);

	/* Close FAT module */
	IOS_Close(fd);

	return ret;
}

s32 Nand_Unmount(nandDevice *dev)
{
	s32 fd, ret;

	/* Open FAT module */
	fd = IOS_Open("fat", 0);
	if (fd < 0)
		return fd;

	/* Unmount device */
	ret = IOS_Ioctlv(fd, dev->umountCmd, 0, 0, NULL);

	/* Close FAT module */
	IOS_Close(fd);

	return ret;
}

s32 Nand_Enable(nandDevice *dev)
{
	s32 fd, ret;

	/* Open /dev/fs */
	fd = IOS_Open("/dev/fs", 0);
	if (fd < 0)
		return fd;

	/* Set input buffer */
	inbuf[0] = dev->mode;

	/* Enable NAND emulator */
	ret = IOS_Ioctl(fd, 100, inbuf, sizeof(inbuf), NULL, 0);

	/* Close /dev/fs */
	IOS_Close(fd);

	return ret;
} 

s32 Nand_Disable(void)
{
	s32 fd, ret;

	/* Open /dev/fs */
	fd = IOS_Open("/dev/fs", 0);
	if (fd < 0)
		return fd;

	/* Set input buffer */
	inbuf[0] = 0;

	/* Disable NAND emulator */
	ret = IOS_Ioctl(fd, 100, inbuf, sizeof(inbuf), NULL, 0);

	/* Close /dev/fs */
	IOS_Close(fd);

	return ret;
} 

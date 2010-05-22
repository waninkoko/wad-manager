/*-------------------------------------------------------------

usbstorage_starlet.c -- USB mass storage support, inside starlet
Copyright (C) 2009 Kwiirk

If this driver is linked before libogc, this will replace the original 
usbstorage driver by svpe from libogc
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/

#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

/* IOCTL commands */
#define UMS_BASE			(('U'<<24)|('M'<<16)|('S'<<8))
#define USB_IOCTL_UMS_INIT	        (UMS_BASE+0x1)
#define USB_IOCTL_UMS_GET_CAPACITY      (UMS_BASE+0x2)
#define USB_IOCTL_UMS_READ_SECTORS      (UMS_BASE+0x3)
#define USB_IOCTL_UMS_WRITE_SECTORS	(UMS_BASE+0x4)
#define USB_IOCTL_UMS_READ_STRESS	(UMS_BASE+0x5)
#define USB_IOCTL_UMS_SET_VERBOSE	(UMS_BASE+0x6)

/* Variables */
static s32 hid = -1;
static s32 fd  = -1;

static u32 sector_size;


inline s32 __USBStorage_isMEM2Buffer(const void *buffer)
{
	u32 high_addr = ((u32)buffer) >> 24;

	return (high_addr == 0x90) || (high_addr == 0xD0);
}


s32 USBStorage_GetCapacity(u32 *_sector_size)
{
	if (fd > 0) {
		s32 ret;

		ret = IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_GET_CAPACITY, ":i", &sector_size);

		if (ret && _sector_size)
			*_sector_size = sector_size;

		return ret;
	}

	return IPC_ENOENT;
}

bool USBStorage_Init(void)
{
	s32 ret;

	/* Already inited */
	if (fd >= 0)
		return true;

	/* Open USB module */
	fd = IOS_Open("/dev/usb2", 0);

	/* Try old module */
	if (fd < 0) {
		fd = IOS_Open("/dev/usb/ehc", 0);
		if (fd < 0)
			return false;
	}

	/* Initialize USB storage */
	IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_INIT, ":");

	/* Get device capacity */
	ret = USBStorage_GetCapacity(NULL);
	if (!ret)
		goto err;

	return true;

err:
	/* Close USB device */
	if (fd > 0) {
		IOS_Close(fd);
		fd = -1;
	}

	return false;
}

bool USBStorage_Deinit(void)
{
	/* Close USB device */
	if (fd >= 0)
		IOS_Close(fd);

	/* Reset descriptor */
	fd = -1;

	return true;
}

bool USBStorage_IsInserted(void)
{
	/* Check if device is inserted */
	return (USBStorage_GetCapacity(NULL) > 0);
}

bool USBStorage_ReadSectors(u32 sector, u32 numSectors, void *buffer)
{
	u32 len = (sector_size * numSectors);

	/* Device not opened */
	if (fd < 0)
		return false;

	/* Read data */
	return IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_READ_SECTORS, "ii:d", sector, numSectors, buffer, len);
}

bool USBStorage_WriteSectors(u32 sector, u32 numSectors, const void *buffer)
{
	u32 len = (sector_size * numSectors);

	/* Device not opened */
	if (fd < 0)
		return false;

	/* Write data */
	return IOS_IoctlvFormat(hid, fd, USB_IOCTL_UMS_WRITE_SECTORS, "ii:d", sector, numSectors, buffer, len);
}

bool USBStorage_ClearStatus(void)
{
	return true;
}


/* Disc interface */
const DISC_INTERFACE __io_usb2storage = {
	DEVICE_TYPE_WII_USB,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_WII_USB,
	(FN_MEDIUM_STARTUP)&USBStorage_Init,
	(FN_MEDIUM_ISINSERTED)&USBStorage_IsInserted,
	(FN_MEDIUM_READSECTORS)&USBStorage_ReadSectors,
	(FN_MEDIUM_WRITESECTORS)&USBStorage_WriteSectors,
	(FN_MEDIUM_CLEARSTATUS)&USBStorage_ClearStatus,
	(FN_MEDIUM_SHUTDOWN)&USBStorage_Deinit
};

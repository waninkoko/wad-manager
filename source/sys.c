#include <stdio.h>
#include <ogcsys.h>

/* Variables */
static const char certs_fs[] ATTRIBUTE_ALIGN(32) = "/sys/cert.sys";


void sys_init(void)
{
	/* Initialize video subsytem */
	VIDEO_Init();
}

void sys_loadmenu(void)
{
	/* Return to the Wii system menu */
	SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}

s32 sys_getcerts(signed_blob **certs, u32 *len)
{
	static signed_blob certificates[0x280] ATTRIBUTE_ALIGN(32);
	s32 fd, ret;

	/* Open certificates file */
	fd = IOS_Open(certs_fs, 1);
	if (fd < 0)
		return fd;

	/* Read certificates */
	ret = IOS_Read(fd, certificates, sizeof(certificates));

	/* Close file */
	IOS_Close(fd);

	/* Set values */
	if (ret > 0) {
		*certs = certificates;
		*len   = sizeof(certificates);
	}

	return ret;
}

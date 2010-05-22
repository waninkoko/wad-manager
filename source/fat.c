#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ogcsys.h>

#include "fat.h"


s32 Fat_Mount(fatDevice *dev)
{
	char dirpath[256];
	s32  ret;

	/* Initialize interface */
	ret = dev->interface->startup();
	if (!ret)
		return -1;

	/* Mount device */
	ret = fatMountSimple(dev->mount, dev->interface);
	if (!ret)
		return -2;

	/* Set root directory */
	sprintf(dirpath, "%s:/", dev->mount);
	chdir(dirpath);

	return 0;
}

void Fat_Unmount(fatDevice *dev)
{
	/* Unmount device */
	fatUnmount(dev->mount);

	/* Shutdown interface */
	dev->interface->shutdown();
}

char *Fat_ToFilename(const char *filename)
{
	static char buffer[128];

	u32 cnt, idx, len;

	/* Clear buffer */
	memset(buffer, 0, sizeof(buffer));

	/* Get filename length */
	len = strlen(filename);

	for (cnt = idx = 0; idx < len; idx++) {
		char c = filename[idx];

		/* Valid characters */
		if ( (c >= '#' && c <= ')') || (c >= '-' && c <= '.') ||
		     (c >= '0' && c <= '9') || (c >= 'A' && c <= 'z') ||
		     (c >= 'a' && c <= 'z') || (c == '!') )
			buffer[cnt++] = c;
	}

	return buffer;
}

void Fat_ChangeDir(const char *dirname)
{
	/* Change directory */
	chdir(dirname);
}

#ifndef _NAND_H_
#define _NAND_H_

/* 'NAND Device' structure */
typedef struct {
	/* Device name */
	char *name;

	/* Mode value */
	u32 mode;

	/* Un/mount command */
	u32 mountCmd;
	u32 umountCmd;
} nandDevice; 


/* Prototypes */
s32 Nand_Mount(nandDevice *);
s32 Nand_Unmount(nandDevice *);
s32 Nand_Enable(nandDevice *);
s32 Nand_Disable(void);

#endif

#ifndef _WAD_H_
#define _WAD_H_

/* WAD header struct */
typedef struct {
	u32 header_len;

	char type[2];
	u16  padding;

	u32 certs_len;
	u32 crl_len;
	u32 tik_len;
	u32 tmd_len;
	u32 data_len;
	u32 footer_len;
} ATTRIBUTE_PACKED wad_header;

/* Prototypes */
s32 WAD_InstallFile(FILE *);
s32 WAD_UninstallFile(FILE *);

#endif

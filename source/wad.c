#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>

#include "video.h"
#include "wad.h"

/* Constant */
#define BLOCK_SIZE	0x1000

/* Macros */
#define round_up(x,n)	(-(-(x) & -(n)))


s32 WAD_FileRead(FILE *fp, void **out, u32 offset, u32 len)
{
	void *buf = NULL;
	s32 ret;

	/* Allocate memory */
	buf = memalign(32, len);
	if (!buf)
		return -1;

	/* Seek to offset */
	fseek(fp, offset, SEEK_SET);

	/* Read data */
	ret = fread(buf, len, 1, fp);
	if (ret != 1) {
		free(buf);
		return -1;
	}

	/* Set pointer */
	*out = buf;

	return 0;
}

s32 WAD_AddTicket(const signed_blob *p_tik, u32 tik_len,
		  const signed_blob *p_certs, u32 certs_len,
		  const signed_blob *p_crl, u32 crl_len)
{
	/* Install ticket */
	return ES_AddTicket(p_tik, tik_len, p_certs, certs_len, p_crl, crl_len);
}

s32 WAD_AddTitleStart(const signed_blob *p_tmd, u32 tmd_len,
		      const signed_blob *p_certs, u32 certs_len,
		      const signed_blob *p_crl, u32 crl_len)
{
	/* Install title */
	return ES_AddTitleStart(p_tmd, tmd_len, p_certs, certs_len, p_crl, crl_len);
}

s32 WAD_AddTitleFinish(void)
{
	/* Finish title install */
	return ES_AddTitleFinish();
}

s32 WAD_AddContentStart(u64 tid, u32 cid)
{
	s32 cfd;

	/* Install content */
	cfd = ES_AddContentStart(tid, cid);
	if (cfd < 0)
		ES_AddTitleCancel();

	return cfd;
}

s32 WAD_AddContentData(s32 cfd, u8 *data, u32 len)
{
	s32 ret;

	/* Install content data */
	ret = ES_AddContentData(cfd, data, len);
	if (ret < 0) {
		ES_AddContentFinish(cfd);
		ES_AddTitleCancel();
	}

	return ret;
}

s32 WAD_AddContentFinish(s32 cfd)
{
	s32 ret;

	/* Finish content install */
	ret = ES_AddContentFinish(cfd);
	if (ret < 0)
		ES_AddTitleCancel();

	return ret;
}

s32 WAD_DeleteTitleContents(u64 tid)
{
	/* Remove title contents */
	return ES_DeleteTitleContent(tid);
}

s32 WAD_DeleteTitle(u64 tid)
{
	/* Delete title */
	return ES_DeleteTitle(tid);
}

s32 WAD_DeleteTicket(u64 tid)
{
	static tikview viewdata[0x10] ATTRIBUTE_ALIGN(32);

	u32 cnt, views;
	s32 ret;

	/* Get number of ticket views */
	ret = ES_GetNumTicketViews(tid, &views);
	if (ret < 0)
		return ret;

	if (!views || views > 16)
		return -1;

	/* Get ticket views */
	ret = ES_GetTicketViews(tid, viewdata, views);
	if (ret < 0)
		return ret;

	/* Remove tickets */
	for (cnt = 0; cnt < views; cnt++) {
		ret = ES_DeleteTicket(&viewdata[cnt]);
		if (ret < 0)
			return ret;
	}

	return 0;
}

s32 WAD_InstallFile(FILE *fp)
{
	wad_header  *header  = NULL;
	signed_blob *p_certs = NULL, *p_crl = NULL, *p_tik = NULL, *p_tmd = NULL;

	tmd *tmd_data = NULL;
	tmd_content *contents = NULL;

	u32 cnt, offset = 0;
	s32 ret;

	con_clearline();

	printf("\r\t\t>> Reading WAD file...");
	fflush(stdout);

	/* WAD header */
	ret = WAD_FileRead(fp, (void *)&header, offset, sizeof(wad_header));
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto out;
	}

	offset += round_up(header->header_len, 64);

	/* WAD certificates */
	ret = WAD_FileRead(fp, (void *)&p_certs, offset, header->certs_len);
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto out;
	}

	offset += round_up(header->certs_len, 64);

	/* WAD crl */
	if (header->crl_len) {
		ret = WAD_FileRead(fp, (void *)&p_crl, offset, header->crl_len);
		if (ret < 0) {
			printf(" ERROR! (ret = %d)\n", ret);
			goto out;
		}
	}

	offset += round_up(header->crl_len, 64);

	/* WAD ticket */
	ret = WAD_FileRead(fp, (void *)&p_tik, offset, header->tik_len);
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto out;
	}

	offset += round_up(header->tik_len, 64);

	/* WAD TMD */
	ret = WAD_FileRead(fp, (void *)&p_tmd, offset, header->tmd_len);
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto out;
	}

	offset += round_up(header->tmd_len, 64);

	con_clearline();

	printf("\t\t>> Installing ticket...");
	fflush(stdout);

	/* Install ticket */
	ret = WAD_AddTicket(p_tik, header->tik_len, p_certs, header->certs_len, p_crl, header->crl_len);
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto out;
	}

	con_clearline();

	printf("\r\t\t>> Installing title...");
	fflush(stdout);

	/* Install title */
	ret = WAD_AddTitleStart(p_tmd, header->tmd_len, p_certs, header->certs_len, p_crl, header->crl_len);
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto out;
	}

	/* Get TMD info */
	tmd_data = (tmd *)SIGNATURE_PAYLOAD(p_tmd);
	contents = TMD_CONTENTS(tmd_data);

	/* Install contents */
	for (cnt = 0; cnt < tmd_data->num_contents; cnt++) {
		tmd_content *content = &contents[cnt];
		u32 content_len = round_up(content->size, 64);

		s32 cfd, idx, len;

		con_clearline();

		printf("\r\t\t>> Installing content #%02d...", content->cid);
		fflush(stdout);

		/* Install content */
		cfd = WAD_AddContentStart(tmd_data->title_id, content->cid);
		if (cfd < 0) {
			printf(" ERROR! (ret = %d)\n", cfd);
			ret = cfd;

			goto out;
		}

		/* Install content data */
		for (idx = 0; idx < content_len; idx += len) {
			u8 *contentBuf = NULL;

			len = (content_len - idx);
			if (len > BLOCK_SIZE)
				len = BLOCK_SIZE;

			/* Read data */
			ret = WAD_FileRead(fp, (void *)&contentBuf, offset, len);
			if (ret < 0) {
				printf(" ERROR! (ret = %d)\n", ret);
				goto out;
			}

			/* Install data */
			ret = WAD_AddContentData(cfd, contentBuf, len);
			if (ret < 0) {
				printf(" ERROR! (ret = %d)\n", ret);
				free(contentBuf);

				goto out;
			}

			/* Free memory */
			free(contentBuf);

			/* Increase offset */
			offset += len;
		}

		/* Finish content installation */
		ret = WAD_AddContentFinish(cfd);
		if (ret < 0) {
			printf(" ERROR! (ret = %d)\n", ret);
			goto out;
		}
	}

	con_clearline();

	printf("\r\t\t>> Finishing title installation...");
	fflush(stdout);

	/* Finish title install */
	ret = WAD_AddTitleFinish();
	if (ret < 0)
		printf(" ERROR! (ret = %d)\n", ret);
	else
		printf(" OK!\n");

out:
	/* Free memory */
	if (header)
		free(header);
	if (p_certs)
		free(p_certs);
	if (p_crl)
		free(p_crl);
	if (p_tik)
		free(p_tik);
	if (p_tmd)
		free(p_tmd);

	return ret;
}

s32 WAD_UninstallFile(FILE *fp)
{
	wad_header  *header = NULL;
	signed_blob *p_tik  = NULL;

	tik *ticket = NULL;

	u32 offset = 0;
	s32 res, ret = 0;

	con_clearline();

	printf("\r\t\t>> Reading WAD file...");
	fflush(stdout);

	/* WAD header */
	ret = WAD_FileRead(fp, (void *)&header, offset, sizeof(wad_header));
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto out;
	}

	/* Jump to ticket */
	offset += round_up(header->header_len, 64);
	offset += round_up(header->certs_len,  64);
	offset += round_up(header->crl_len,    64);

	/* WAD ticket */
	ret = WAD_FileRead(fp, (void *)&p_tik, offset, header->tik_len);
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto out;
	}

	/* Get ticket info */
	ticket = (tik *)SIGNATURE_PAYLOAD(p_tik);

	con_clearline();

	printf("\r\t\t>> Deleting ticket...");
	fflush(stdout);

	/* Delete title tickets */
	res = WAD_DeleteTicket(ticket->titleid);
	if (res < 0) {
		printf(" ERROR! (ret = %d)\n", res);
		ret = res;
	} else
		printf(" OK!\n");

	con_clearline();

	printf("\r\t\t>> Deleting title contents...");
	fflush(stdout);

	/* Delete title contents */
	res = WAD_DeleteTitleContents(ticket->titleid);
	if (res < 0) {
		printf(" ERROR! (ret = %d)\n", res);
		ret = res;

		goto out;
	}

	con_clearline();

	printf("\r\t\t>> Deleting title...");
	fflush(stdout);

	/* Delete title */
	res = WAD_DeleteTitle(ticket->titleid);
	if (res < 0) {
		printf(" ERROR! (ret = %d)\n", res);
		ret = res;
	} else
		printf(" OK!\n");

out:
	/* Free memory */
	if (header)
		free(header);
	if (p_tik)
		free(p_tik);

	return ret;
}

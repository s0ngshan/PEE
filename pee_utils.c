#include "pee_utils.h"
#include <stdio.h>
#include <stdlib.h>

//open_check
int File_open(const char* path, uint8_t** out_buf, size_t* out_size) {
	if (!path || !out_buf || !out_size) return 0;

	*out_buf = NULL;
	*out_size = 0;

	FILE* fp = fopen(path, "rb");                     //以二进制只读模式打开文件
	if (!fp) return 0;

	if (fseek(fp, 0, SEEK_END) != 0) {                //将文件指针移到文件末尾，移动成功会返回0
		fclose(fp);
		return 0;
	}

	long fsize_long = ftell(fp);                      //ftell会返回当前文件指针位置，此时返回文件大小
	if (fsize_long < 0) {
		fclose(fp);
		return 0;
	}

	size_t fsize = (size_t)fsize_long;
	rewind(fp);                                       //将文件指针重新指向文件开头

	if (fsize < 0x40) {                               //3C到40是e_lfanew所占的四字节，至少保证要读到这里
		fclose(fp);
		return 0;
	}

	uint8_t* buf = (uint8_t*)malloc(fsize);
	if (!buf) {
		fclose(fp);
		return 0;
	}

	size_t readn = fread(buf, 1, fsize, fp);
	fclose(fp);

	if (readn != fsize) {
		free(buf);
		return 0;
	}

	*out_buf = buf;
	*out_size = fsize;
	return 1;
}

int PE_Check(const uint8_t* buf, size_t size, uint32_t* out_e_lfanew) {
	if (!buf || size < 0x40) return 0;

	if (buf[0] != 'M' || buf[1] != 'Z') return 0;

	uint32_t e_lfanew =
		(uint32_t)buf[0x3C] |
		((uint32_t)buf[0x3D] << 8) |
		((uint32_t)buf[0x3E] << 16) |
		((uint32_t)buf[0x3F] << 24);

	if ((size_t)e_lfanew + 4 > size) return 0;

	if (buf[e_lfanew] != 'P' ||
		buf[e_lfanew + 1] != 'E' ||
		buf[e_lfanew + 2] != 0x00 ||
		buf[e_lfanew + 3] != 0x00) {
		return 0;
	}

	if (out_e_lfanew) {
		*out_e_lfanew = e_lfanew;
	}

	return 1;
}

//RVA_VA
static uint16_t rd16(const uint8_t* p) {
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t rd32(const uint8_t* p) {
	return (uint32_t)p[0] |
		((uint32_t)p[1] << 8) |
		((uint32_t)p[2] << 16) |
		((uint32_t)p[3] << 24);
}

static uint64_t rd64(const uint8_t* p) {
	return (uint64_t)p[0] |
		((uint64_t)p[1] << 8) |
		((uint64_t)p[2] << 16) |
		((uint64_t)p[3] << 24) |
		((uint64_t)p[4] << 32) |
		((uint64_t)p[5] << 40) |
		((uint64_t)p[6] << 48) |
		((uint64_t)p[7] << 56);
}

int Entrypoint_Rva(const uint8_t* buf, size_t size, uint32_t* out_rva) {
	if (!out_rva) return 0;

	uint32_t e_lfanew = 0;
	if (!PE_Check(buf, size, &e_lfanew)) return 0;

	size_t opt_off = (size_t)e_lfanew + 24;
	if (opt_off + 0x14 > size) return 0;

	uint16_t magic = rd16(buf + opt_off);
	if (magic != 0x10B && magic != 0x20B) return 0;

	*out_rva = rd32(buf + opt_off + 0x10);
	return 1;
}

int Imagebase_Get(const uint8_t* buf, size_t size, uint64_t* out_image_base) {
	if (!out_image_base) return 0;

	uint32_t e_lfanew = 0;
	if (!PE_Check(buf, size, &e_lfanew)) return 0;

	size_t opt_off = (size_t)e_lfanew + 24;
	if (opt_off + 2 > size) return 0;

	uint16_t magic = rd16(buf + opt_off);

	if (magic == 0x10B) {
		if (opt_off + 0x20 > size) return 0;
		*out_image_base = (uint64_t)rd32(buf + opt_off + 0x1C);
		return 1;
	}
	else if (magic == 0x20B) {
		if (opt_off + 0x20 > size) return 0;
		*out_image_base = rd64(buf + opt_off + 0x18);
		return 1;
	}

	return 0;
}

int Rva_To_Va(uint32_t rva, uint64_t image_base, uint64_t* out_va) {
	if (!out_va) return 0;
	*out_va = image_base + (uint64_t)rva;
	return 1;
}

#include "pee_utils.h"
#include <stdio.h>
#include <stdlib.h>

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



}

int PE_Check(const uint8_t* buf, size_t size, uint32_t* out_e_lfanew) {

}

#include <stdio.h>
#include <stdlib.h>
#include "pee_utils.h"
#include <windows.h>
#include <locale.h>

int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	char path[1024] = {0};               //定义文件路径缓冲区存放

	uint32_t entry_rva = 0;
	uint64_t image_base = 0;
	uint64_t entry_va = 0;

	//优先读取命令行参数
	if (argc >= 2) {
		snprintf(path, sizeof(path), "%s", argv[1]);
	}
	else {
		printf("输入文件路径：");
		if (!fgets(path, sizeof(path), stdin)) {
			printf("读取输入失败\n");
			return 1;
		}

		for (int i = 0; path[i]; i++) {
			if (path[i] == '\n' || path[i] == '\r') {
				path[i] = '\0';
				break;
			}
		}
	}

	uint8_t* buf = NULL;
	size_t size = 0;

	if (!File_open(path, &buf, &size)) {
		printf("打开文件失败：%s\n", path);
		return 1;
	}

	uint32_t e_lfanew = 0;
	if (!PE_Check(buf, size, &e_lfanew)) {
		printf("不是有效PE文件\n");
		free(buf);
		return 1;
	}

	printf("正确的PE标识\n");
	printf("e_lfanew = 0x%08X\n",e_lfanew);

	if (!Entrypoint_Rva(buf, size, &entry_rva)) {
		printf("读取EntryPoint Rva 失败\n");
		free(buf);
		return 1;
	}
	if (!Imagebase_Get(buf, size, &image_base)) {
		printf("\n");
		free(buf);
		return 1;
	}
	if (!Rva_To_Va(entry_rva, image_base, &entry_va)) {
		printf("\n");
		free(buf);
		return 1;
	}

	printf("EntryPoint RVA = 0x%08X\n",entry_rva);
	printf("ImageBase      = 0x%I64X\n",(unsigned long long)image_base);
	printf("EntryPoint VA  = 0x%I64X\n",(unsigned long long)entry_va);

	free(buf);
	return 0;
}
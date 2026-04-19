#pragma once

#include <stddef.h>
#include <stdint.h>

int File_open(const char* path, uint8_t ** out_buf, size_t* out_size);
int PE_Check(const uint8_t* buf, size_t size, uint32_t* out_e_lfanew);
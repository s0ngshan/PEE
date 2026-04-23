#pragma once

#include <stddef.h>
#include <stdint.h>

//1.0.0
int File_open(const char* path, uint8_t ** out_buf, size_t* out_size);
int PE_Check(const uint8_t* buf, size_t size, uint32_t* out_e_lfanew);

//1.1.0
int Entrypoint_Rva(const uint8_t* buf, size_t size, uint32_t* out_rva);
int Imagebase_Get(const uint8_t* buf,size_t size,uint64_t* out_image_base);
int Rva_To_Va(uint32_t rva,uint64_t image_base,uint64_t* out_va);

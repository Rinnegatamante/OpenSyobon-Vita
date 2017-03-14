#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vitasdk.h>
#include "filesystem.h"


FILE* openFile(const char* fn, const char* mode)
{
	if(!fn || !mode)return NULL;
	return fopen(fn, mode);
}

void* bufferizeFile(const char* filename, uint32_t* size, bool binary)
{
	FILE* file;
	
	if (!binary) file = openFile(filename, "r");
	else file = openFile(filename, "rb");
	
	if (!file) return NULL;
	
	uint8_t* buffer;
	long lsize;
	fseek (file, 0, SEEK_END);
	lsize = ftell(file);
	fseek(file, 0, SEEK_SET);
	buffer=(uint8_t*)malloc(lsize);
	if (size) *size = lsize;
	
	if (!buffer){
		fclose(file);
		return NULL;
	}
		
	fread(buffer, 1, lsize, file);
	fclose(file);
	return buffer;
}

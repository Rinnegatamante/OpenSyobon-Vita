#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <vitasdk.h>

FILE* openFile(const char* fn, const char* mode);
void* bufferizeFile(const char* filename, uint32_t* size, bool binary);

#endif
#include "filedata.h"

#include <cstring>
#include <iostream>

FileData::FileData(const std::string &name) :
    filename(name),
    buf(NULL),
    size(0)
{
    readFile();
}

FileData::~FileData() {
    free(buf);
}

void FileData::padBuffer(int padding)
{
    if (padding > 0) {
        buf = (char*) realloc(buf, (size+padding)*sizeof(char));
        if (buf == NULL) {
            std::cerr << "Error: realloc() failed" << std::endl;
            exit(EXIT_FAILURE);
        }
        memset(buf+size, 0, padding);
    }
}

void FileData::readFile()
{
    std::ifstream f(filename, std::ios::in | std::ios::binary);

    f.seekg(0, std::ios::end);
    size = f.tellg();
    if (size > maxsize || size < 0) {
        size = 0;
        return;
    }
    f.seekg(0, std::ios::beg);

    buf = (char*) malloc(size*sizeof(char));
    if (buf == NULL) {
        std::cerr << "Error: malloc() failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    f.read(buf, size);
}

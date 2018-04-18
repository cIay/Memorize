#ifndef FILEDATA_H
#define FILEDATA_H

#include <fstream>

class FileData
{
public:
    FileData(const std::string&);
    ~FileData();
    void padBuffer(int);
    char* getBuffer() { return buf; }
    unsigned long getSize(){ return size; }

private:
    void readFile(const std::string&);
    char *buf;
    unsigned long size;
};

#endif // FILEDATA_H

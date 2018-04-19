#ifndef FILEDATA_H
#define FILEDATA_H

#include <fstream>

class FileData
{
public:
    FileData(const std::string&);
    ~FileData();
    std::string getFilename(){ return filename; }
    void padBuffer(int);
    char* getBuffer() { return buf; }
    long getSize() { return size; }
    long getMaxsize() { return maxsize; }

private:
    void readFile();
    const std::string filename;
    char *buf;
    long size;
    const long maxsize = 400000000; // 400mb
};

#endif // FILEDATA_H

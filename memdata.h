#ifndef MEMDATA_H
#define MEMDATA_H

#include <windows.h>
#include <iostream>
#include <vector>

class MemData
{
public:
    MemData(DWORD);
    ~MemData();
    void padBuffer(int);
    static void fillProcLists();
    static std::vector<std::wstring> getProcNamelist() { return proc_namelist; }
    static std::vector<DWORD> getProcIDlist() {return proc_idlist; }
    unsigned char* getBuffer() { return buf; }
    long getSize(){ return size; }
    void* findAddr(long index) { return (index < size) ? addrbuf[index] : NULL; }

private:
    void readMem(DWORD);
    static std::vector<std::wstring> proc_namelist;
    static std::vector<DWORD> proc_idlist;
    unsigned char *buf;
    long size;
    void **addrbuf;
    const long maxsize = 600000000; // 600mb
};

#endif // MEMDATA_H

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
    void* findAddr(long);

private:
    void readMem(DWORD);
    static std::vector<std::wstring> proc_namelist;
    static std::vector<DWORD> proc_idlist;
    unsigned char *buf;
    long size;
    const long maxsize = 1000000000; // 1000mb
    void **regions;
    SIZE_T *region_sizes;
    int n_regions;
};

#endif // MEMDATA_H

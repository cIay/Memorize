#include "memdata.h"

#include <tlhelp32.h>

std::vector<std::wstring> MemData::proc_namelist;
std::vector<DWORD> MemData::proc_idlist;

MemData::MemData(DWORD pid) :
    buf(NULL),
    size(0),
    addrbuf(NULL)
{
    readMem(pid);
}

MemData::~MemData() {
    free(buf);
    free(addrbuf);
}

void MemData::padBuffer(int padding)
{
    if (padding > 0) {
        buf = (unsigned char*) realloc(buf, (size+padding)*sizeof(unsigned char));
        if (buf == NULL) {
            std::cerr << "Error: realloc() failed" << std::endl;
            exit(EXIT_FAILURE);
        }
        memset(buf+size, 0, padding);
    }
}

void MemData::fillProcLists()
{
    HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hsnap == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateToolhelp32Snapshot() failed with error " + std::to_string(GetLastError())<< std::endl;
        exit(EXIT_FAILURE);
    }
    PROCESSENTRY32 proc_entry;
    proc_entry.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hsnap, &proc_entry)) {
        std::cerr << "Process32First() failed with error " + std::to_string(GetLastError()) << std::endl;
        exit(EXIT_FAILURE);
    }

    proc_namelist.clear();
    proc_idlist.clear();
    do {
        std::wstring ws(proc_entry.szExeFile);
        proc_namelist.push_back(ws);
        proc_idlist.push_back(proc_entry.th32ProcessID);
    } while (Process32Next(hsnap, &proc_entry));
}

void MemData::readMem(DWORD pid)
{
    HANDLE hproc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, false, pid);
    if (hproc == NULL) {
        std::cerr << "OpenProcess() failed with error " + std::to_string(GetLastError()) << std::endl;
        return;
    }

    MEMORY_BASIC_INFORMATION info;
    unsigned char *addr = NULL;

    int n_regions = 0;
    long est_size = 0;
    while (VirtualQueryEx(hproc, addr, &info, sizeof(info)) == sizeof(info)) {
        if (info.Protect == PAGE_READWRITE && info.Type == MEM_PRIVATE) {
            est_size += info.RegionSize;
        }
        addr += info.RegionSize;
        n_regions++;
    }
    buf = (unsigned char*) malloc(est_size * sizeof(unsigned char));
    if (buf == NULL) {
        std::cerr << "Error: realloc() failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    addrbuf = (void**) malloc(est_size * sizeof(void*));
    if (addrbuf == NULL) {
        std::cerr << "Error: realloc() failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    addr = NULL;

    while (VirtualQueryEx(hproc, addr, &info, sizeof(info)) == sizeof(info)) {
        if (info.Protect == PAGE_READWRITE && info.Type == MEM_PRIVATE) {
            size += info.RegionSize;
            if (size > est_size) {
                buf = (unsigned char*) realloc(buf, size * sizeof(unsigned char));
                if (buf == NULL) {
                    std::cerr << "Error: realloc() failed" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            SIZE_T bytes_read;
            if (ReadProcessMemory(hproc, addr, buf + (size-info.RegionSize), info.RegionSize, &bytes_read) == 0) {
                std::cerr << "Warning: ReadProcessMemory() failed with error " + std::to_string(GetLastError()) << std::endl;
                size -= info.RegionSize;
                break;
            }
            else {
                if (size > est_size) {
                    addrbuf = (void**) realloc(addrbuf, size * sizeof(void*));
                    if (addrbuf == NULL) {
                        std::cerr << "Error: realloc() failed" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
                for (unsigned int i = 0; i < info.RegionSize; i++) {
                    addrbuf[size-info.RegionSize+i] = addr+i;
                }
            }
        }
        addr += info.RegionSize;
    }
}

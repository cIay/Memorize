#include "memdata.h"

#include <tlhelp32.h>

#include <QDebug.h>

std::vector<std::wstring> MemData::proc_namelist;
std::vector<DWORD> MemData::proc_idlist;

MemData::MemData(DWORD pid) :
    buf(NULL),
    size(0),
    regions(NULL),
    region_sizes(NULL)
{
    readMem(pid);
}

MemData::~MemData() {
    free(buf);
    free(regions);
    free(region_sizes);
}

void MemData::cmpBytes(MemData *md, bool diff)
{
    size = 0;
    int new_n_regions = 0;
    int max_n_regions = n_regions;
    void **new_regions = (void**) malloc(max_n_regions * sizeof(void*));
    if (new_regions == NULL) {
        std::cerr << "Error: malloc() failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    SIZE_T *new_region_sizes = (SIZE_T*) malloc(max_n_regions * sizeof(SIZE_T));
    if (new_region_sizes == NULL) {
        std::cerr << "Error: malloc() failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    long index = 0;
    for (int i = 0; i < n_regions; i++) {
        bool newblock = true;
        for (SIZE_T j = 0; j < region_sizes[i]; j++) {
            void *addr = (unsigned char*) regions[i] + j;
            unsigned char byte = buf[index + j];
            unsigned char newbyte = byte;
            bool equal = true;
            long cmp_index = 0;
            for (int k = 0; k < md->getNRegions(); k++) {
                if (addr >= md->getRegions()[k] && addr < (unsigned char*) md->getRegions()[k] + md->getRegionSizes()[k]) {
                    long long offset = (unsigned char*) addr - (unsigned char*) md->getRegions()[k];
                    if (diff) {
                        if (byte != md->getBuffer()[cmp_index + offset]) {
                            /*
                            qDebug() << "byte:" << (int)byte
                                     << " cmpbyte:" << (int)md->getBuffer()[cmp_index + offset]
                                     << " pos:" << index + j
                                     << " cmppos:" << cmp_index + offset
                                     << " addr:" << addr
                                     << " cmpaddr:" << (unsigned char*) md->getRegions()[k] + offset;
                                     */
                            newbyte = (int)md->getBuffer()[cmp_index + offset];
                            equal = false;
                        }
                    }
                    else {
                        if (byte == md->getBuffer()[cmp_index + offset]) {
                            newbyte = (int)md->getBuffer()[cmp_index + offset];
                            equal = false;
                        }
                    }
                    break;
                }
                cmp_index += md->getRegionSizes()[k];
            }

            if (!equal) {
                if (newblock) {
                    new_n_regions++;
                    if (new_n_regions > max_n_regions) {
                        max_n_regions *= 2;
                        new_regions = (void**) realloc(new_regions, max_n_regions * sizeof(void*));
                        if (new_regions == NULL) {
                            std::cerr << "Error: realloc() failed" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        new_region_sizes = (SIZE_T*) realloc(new_region_sizes, max_n_regions * sizeof(SIZE_T));
                        if (new_region_sizes == NULL) {
                            std::cerr << "Error: realloc() failed" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                    }
                    new_regions[new_n_regions-1] = addr;
                    new_region_sizes[new_n_regions-1] = 1;
                }
                else {
                    new_region_sizes[new_n_regions-1]++;
                }
                buf[size] = newbyte;
                size++;
                newblock = false;
            }
            else {
                newblock = true;
            }
        }
        index += region_sizes[i];
    }
    free(regions); regions = new_regions;
    free(region_sizes); region_sizes = new_region_sizes;
    n_regions = new_n_regions;
    if (size != 0) {
        buf = (unsigned char*) realloc(buf, size*sizeof(unsigned char));
        if (buf == NULL) {
            std::cerr << "Error: realloc() failed" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    else {
        buf = NULL;
    }
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

void* MemData::findAddr(long index)
{
    int running_total = 0;
    for (int i = 0; i < n_regions; i++) {
        running_total += region_sizes[i];
        if (index < running_total) {
            long offset = index - (running_total - region_sizes[i]);
            return ((unsigned char*)regions[i]) + offset;
        }
    }
    return NULL;
}

void MemData::readMem(DWORD pid)
{
    HANDLE hproc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, false, pid);
    if (hproc == NULL) {
        std::cerr << "OpenProcess() failed with error " + std::to_string(GetLastError()) << std::endl;
        free(buf); buf = NULL;
        return;
    }

    MEMORY_BASIC_INFORMATION info;
    unsigned char *addr = NULL;

    long est_size = 0;
    int est_regions = 0;
    while (VirtualQueryEx(hproc, addr, &info, sizeof(info)) == sizeof(info)) {
        if (info.Protect == PAGE_READWRITE && info.Type == MEM_PRIVATE) {
            est_size += info.RegionSize;
        }
        addr += info.RegionSize;
        est_regions++;
    }

    if (est_regions == 0) {
        free(buf); buf = NULL;
        return;
    }

    buf = (unsigned char*) malloc(est_size * sizeof(unsigned char));
    if (buf == NULL) {
        std::cerr << "Error: realloc() failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    regions = (void**) malloc(est_regions * sizeof(void*));
    if (regions == NULL) {
        std::cerr << "Error: malloc() failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    region_sizes = (SIZE_T*) malloc(est_regions * sizeof(SIZE_T));
    if (region_sizes == NULL) {
        std::cerr << "Error: malloc() failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    addr = NULL;
    n_regions = 0;

    while (VirtualQueryEx(hproc, addr, &info, sizeof(info)) == sizeof(info)) {
        if (info.Protect == PAGE_READWRITE && info.Type == MEM_PRIVATE) {
            size += info.RegionSize;
            if (size > maxsize) {
                std::cerr << "Warning: Memory limit reached" << std::endl;
                size -= info.RegionSize;
                break;
            }
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
                continue;
            }

            n_regions++;
            if (n_regions > est_regions) {
                regions = (void**) realloc(regions, n_regions * sizeof(void*));
                if (regions == NULL) {
                    std::cerr << "Error: realloc() failed" << std::endl;
                    exit(EXIT_FAILURE);
                }
                region_sizes = (SIZE_T*) realloc(region_sizes, n_regions * sizeof(SIZE_T));
                if (regions == NULL) {
                    std::cerr << "Error: realloc() failed" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            regions[n_regions-1] = addr;
            region_sizes[n_regions-1] = info.RegionSize;
        }
        addr += info.RegionSize;
    }
}

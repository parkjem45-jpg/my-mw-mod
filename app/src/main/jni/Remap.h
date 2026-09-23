#pragma once
#include <link.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace RemapTools {

class ProcMapInfo {
   private:
    uintptr_t start;
    uintptr_t end;
    uintptr_t offset;
    uint8_t perms;
    ino_t inode;
    std::string dev;
    std::string path;

    friend class Remapper;
};

class Remapper {
   private:
    static int iteratePhdrCallback(struct dl_phdr_info* info, size_t size, void* data);
    static std::vector<ProcMapInfo> ListModulesWithNameImpl(const std::string& name);
    static void RemapLibraryImpl(const char* libName);

   public:
    Remapper() = default;
    ~Remapper() = default;
    static void Remap(const char* libname);
};

std::vector<ProcMapInfo> Remapper::ListModulesWithNameImpl(const std::string& name) {
    std::vector<ProcMapInfo> result;
    FILE* fp = fopen(oxorany("/proc/self/maps"), "r");
    if (!fp)
        return result;

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, name.c_str())) {
            ProcMapInfo info{};
            char perms[10];
            char dev[32];
            char path[256] = {0};

            
            if (sscanf(buffer, OBFUSCATE("%lx-%lx %s %lx %31s %lu %255s"), &info.start, &info.end, perms, &info.offset, dev,
                       &info.inode, path) >= 6) {
                info.perms = 0;
                if (strchr(perms, 'r'))
                    info.perms |= PROT_READ;
                if (strchr(perms, 'w'))
                    info.perms |= PROT_WRITE;
                if (strchr(perms, 'x'))
                    info.perms |= PROT_EXEC;

                info.dev = dev;
                info.path = path;

                result.push_back(info);
            }
        }
    }
    fclose(fp);
    return result;
}

int Remapper::iteratePhdrCallback(struct dl_phdr_info* info, size_t size, void* data) {
    const char* libraryName = static_cast<const char*>(data);
    if (info->dlpi_name && strstr(info->dlpi_name, libraryName)) {
        auto maps = Remapper::ListModulesWithNameImpl(libraryName);
        for (auto& m : maps) {
            size_t regionSize = m.end - m.start;
            void* address = reinterpret_cast<void*>(m.start);

            
            if (m.perms & PROT_EXEC)
                continue;

            void* map = mmap(nullptr, regionSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
            if (map == MAP_FAILED) {
                perror(OBFUSCATE("mmap failed"));
                continue;
            }

            std::memcpy(map, address, regionSize);

            if (mremap(map, regionSize, regionSize, MREMAP_MAYMOVE | MREMAP_FIXED, address) == MAP_FAILED) {
                perror(OBFUSCATE("mremap failed"));
                munmap(map, regionSize);
                continue;
            }

            mprotect(address, regionSize, m.perms);
        }
    }
    return 0;
}

void Remapper::RemapLibraryImpl(const char* libName) {
    dl_iterate_phdr(iteratePhdrCallback, const_cast<char*>(libName));
}

void Remapper::Remap(const char* libname) {
    RemapLibraryImpl(libname);
}

} 
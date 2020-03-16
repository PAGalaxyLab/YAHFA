//
// Created by liuruikai756 on 2020/3/13.
//

#define _GNU_SOURCE
#include <link.h>

#include <elf.h>
#include <string.h>
#include <dlfcn.h>
#include "common.h"

#define LINKER "/system/bin/linker"

#if defined(__i386__) || defined(__arm__)
#define LIBNATIVELOADER "/apex/com.android.art/lib/libnativeloader.so"
#else
#define LIBNATIVELOADER "/apex/com.android.art/lib64/libnativeloader.so"
#endif

static void init() __attribute__((constructor));

typedef void * (*dlopenFunc)(const char *name, int flags, const void *caller_addr);
typedef void * (*dlsymFunc)(void* handle, const char* symbol, const void* caller_addr);

static dlopenFunc __loader_dlopen = NULL;
static dlsymFunc __loader_dlsym = NULL;
static ElfW(Addr) libnativeloader_base = 0;

static int callback(struct dl_phdr_info *info, size_t size, void *data) {
    int p_type, j;

    if(strcmp(info->dlpi_name, LIBNATIVELOADER) == 0) {
        libnativeloader_base = info->dlpi_addr;
        return 0;
    }
    else if(strncmp(info->dlpi_name, LINKER, sizeof(LINKER)-1) == 0) {
        ElfW(Sym) *linker_dynsym = NULL;
        ElfW(Addr) linker_dynstr = NULL;
        for (j = 0; j < info->dlpi_phnum; j++) {
            p_type = info->dlpi_phdr[j].p_type;
            if(p_type == PT_DYNAMIC) {
                ElfW(Dyn) *linker_dyn = info->dlpi_addr + info->dlpi_phdr[j].p_vaddr;
                for (; linker_dyn->d_tag; linker_dyn++) {
                    if (linker_dyn->d_tag == DT_SYMTAB) {
                        linker_dynsym = info->dlpi_addr + linker_dyn->d_un.d_val;
                    } else if (linker_dyn->d_tag == DT_STRTAB) {
                        linker_dynstr = info->dlpi_addr + linker_dyn->d_un.d_val;
                    }
                }

                for (;; linker_dynsym++) {
                    if (strcmp(linker_dynstr + linker_dynsym->st_name, "__loader_dlopen") == 0) {
                        __loader_dlopen = info->dlpi_addr + linker_dynsym->st_value;
                    } else if (strcmp(linker_dynstr + linker_dynsym->st_name, "__loader_dlsym") == 0) {
                        __loader_dlsym = info->dlpi_addr + linker_dynsym->st_value;
                    }

                    if (__loader_dlopen != NULL && __loader_dlsym != NULL) {
                        break;
                    }
                }



                break;
            }
        }

        return 0;
    }
    else if(__loader_dlopen != NULL
            && __loader_dlsym != NULL
            && libnativeloader_base != 0
    ) {
        return 1; // all found, stop the dl_iterate_phdr iteration
    }
    else {
        return 0;
    }
}

static void init() {
    dl_iterate_phdr(callback, NULL);
    LOGI("__loader_dlopen is at %p, __loader_dlsym is at %p, libnativeloader base is at %p",
            __loader_dlopen, __loader_dlsym, libnativeloader_base);
}

void *art_dlopen(const char *name, int flags) {
    if(__loader_dlopen != NULL
       && __loader_dlsym != NULL
       && libnativeloader_base != 0) {
        return (*__loader_dlopen)(name, flags, libnativeloader_base);
    }
    else {
        return NULL;
    }
}

void *art_dlsym(void *handle, const char *name) {
    if(handle == NULL) {
        LOGW("handle is null for symbol %s", name);
        return NULL;
    }
    if(__loader_dlopen != NULL
       && __loader_dlsym != NULL
       && libnativeloader_base != 0) {
        return (*__loader_dlsym)(handle, name, libnativeloader_base);
    }
    else {
        return NULL;
    }
}
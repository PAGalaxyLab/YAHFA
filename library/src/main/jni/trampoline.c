//
// Created by liuruikai756 on 05/07/2017.
//
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "common.h"
#include "trampoline.h"

static unsigned char *currentTrampolineOff = 0;
static unsigned char *trampolineSpaceEnd = 0;

static void *allocTrampolineSpace();

// trampoline:
// 1. set eax/rdi/r0/x0 to the hook ArtMethod addr
// 2. jump into its entry point

// trampoline for backup:
// 1. set eax/rdi/r0/x0 to the target ArtMethod addr
// 2. ret to the hardcoded original entry point

#if defined(__i386__)
// b8 78 56 34 12 ; mov eax, 0x12345678 (addr of the hook method)
// ff 70 20 ; push DWORD PTR [eax + 0x20]
// c3 ; ret
unsigned char trampoline[] = {
        0x00, 0x00, 0x00, 0x00, // code_size_ in OatQuickMethodHeader
        0xb8, 0x78, 0x56, 0x34, 0x12,
        0xff, 0x70, 0x20,
        0xc3
};

// b8 78 56 34 12 ; mov eax, 0x12345678 (addr of the target method)
// 68 78 56 34 12 ; push 0x12345678 (original entry point of the target method)
// c3 ; ret
unsigned char trampolineForBackup[] = {
        0xb8, 0x78, 0x56, 0x34, 0x12,
        0x68, 0x78, 0x56, 0x34, 0x12,
        0xc3
};

#elif defined(__x86_64__)
// 48 bf 78 56 34 12 78 56 34 12 ; movabs rdi, 0x1234567812345678
// ff 77 20 ; push QWORD PTR [rdi + 0x20]
// c3 ; ret
unsigned char trampoline[] = {
    0x00, 0x00, 0x00, 0x00, // code_size_ in OatQuickMethodHeader
    0x48, 0xbf, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12,
    0xff, 0x77, 0x20,
    0xc3
};

// 48 bf 78 56 34 12 78 56 34 12 ; movabs rdi, 0x1234567812345678
// 57 ; push rdi (original entry point of the target method)
// 48 bf 78 56 34 12 78 56 34 12 ; movabs rdi, 0x1234567812345678 (addr of the target method)
// c3 ; ret
unsigned char trampolineForBackup[] = {
    0x48, 0xbf, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12,
    0x57,
    0x48, 0xbf, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12,
    0xc3
};

#elif defined(__arm__)
// 00 00 9F E5 ; ldr r0, [pc, #0]
// 20 F0 90 E5 ; ldr pc, [r0, 0x20]
// 78 56 34 12 ; 0x12345678 (addr of the hook method)
unsigned char trampoline[] = {
        0x00, 0x00, 0x00, 0x00, // code_size_ in OatQuickMethodHeader
        0x00, 0x00, 0x9f, 0xe5,
        0x20, 0xf0, 0x90, 0xe5,
        0x78, 0x56, 0x34, 0x12
};

// 0c 00 9F E5 ; ldr r0, [pc, #12]
// 01 00 2d e9 ; push {r0}
// 00 00 9F E5 ; ldr r0, [pc, #0]
// 00 80 bd e8 ; pop {pc}
// 78 56 34 12 ; 0x12345678 (addr of the hook method)
// 78 56 34 12 ; 0x12345678 (original entry point of the target method)
unsigned char trampolineForBackup[] = {
        0x0c, 0x00, 0x9f, 0xe5,
        0x01, 0x00, 0x2d, 0xe9,
        0x00, 0x00, 0x9f, 0xe5,
        0x00, 0x80, 0xbd, 0xe8,
        0x78, 0x56, 0x34, 0x12,
        0x78, 0x56, 0x34, 0x12
};

#elif defined(__aarch64__)
// 60 00 00 58 ; ldr x0, 12
// 10 00 40 F8 ; ldr x16, [x0, #0x00]
// 00 02 1f d6 ; br x16
// 78 56 34 12
// 89 67 45 23 ; 0x2345678912345678 (addr of the hook method)
unsigned char trampoline[] = {
        0x00, 0x00, 0x00, 0x00, // code_size_ in OatQuickMethodHeader
        0x60, 0x00, 0x00, 0x58,
        0x10, 0x00, 0x40, 0xf8,
        0x00, 0x02, 0x1f, 0xd6,
        0x78, 0x56, 0x34, 0x12,
        0x89, 0x67, 0x45, 0x23
};

// 60 00 00 58 ; ldr x0, 12
// 90 00 00 58 ; ldr x16, 16
// 00 02 1f d6 ; br x16
// 78 56 34 12
// 89 67 45 23 ; 0x2345678912345678 (addr of the hook method)
// 78 56 34 12
// 89 67 45 23 ; 0x2345678912345678 (original entry point of the target method)
unsigned char trampolineForBackup[] = {
        0x60, 0x00, 0x00, 0x58,
        0x90, 0x00, 0x00, 0x58,
        0x00, 0x02, 0x1f, 0xd6,
        0x78, 0x56, 0x34, 0x12,
        0x89, 0x67, 0x45, 0x23,
        0x78, 0x56, 0x34, 0x12,
        0x89, 0x67, 0x45, 0x23
};
#endif

void *genTrampoline(void *toMethod, void *entrypoint) {
    size_t trampolineSize = entrypoint != NULL ? sizeof(trampolineForBackup) : sizeof(trampoline);

    // check available space for new trampoline
    if(currentTrampolineOff+trampolineSize > trampolineSpaceEnd) {
        currentTrampolineOff = allocTrampolineSpace();
        if (currentTrampolineOff == NULL) {
            return NULL;
        } else {
            trampolineSpaceEnd = currentTrampolineOff + TRAMPOLINE_SPACE_SIZE;
        }
    }

    unsigned char *targetAddr = currentTrampolineOff;

    if(entrypoint != NULL) {
        memcpy(targetAddr, trampolineForBackup, sizeof(trampolineForBackup));
    }
    else {
        memcpy(targetAddr, trampoline,
               sizeof(trampoline)); // do not use trampolineSize since it's a rounded size
    }

    // replace with the actual ArtMethod addr
#if defined(__i386__)
    if(entrypoint) {
        memcpy(targetAddr + 1, &toMethod, pointer_size);
        memcpy(targetAddr + 6, &entrypoint, pointer_size);
    }
    else {
        memcpy(targetAddr + 5, &toMethod, pointer_size);
    }

#elif defined(__x86_64__)
    if(entrypoint) {
        memcpy(targetAddr + 2, &entrypoint, pointer_size);
        memcpy(targetAddr + 13, &toMethod, pointer_size);
    }
    else {
        memcpy(targetAddr + 6, &toMethod, pointer_size);
    }

#elif defined(__arm__)
    if(entrypoint) {
        memcpy(targetAddr + 20, &entrypoint, pointer_size);
        memcpy(targetAddr + 16, &toMethod, pointer_size);
    }
    else {
        memcpy(targetAddr + 12, &toMethod, pointer_size);
    }

#elif defined(__aarch64__)
    if(entrypoint) {
        memcpy(targetAddr + 20, &entrypoint, pointer_size);
        memcpy(targetAddr + 12, &toMethod, pointer_size);
    }
    else {
        memcpy(targetAddr + 16, &toMethod, pointer_size);
    }
#else
#error Unsupported architecture
#endif

    // skip 4 bytes of code_size_
    if(entrypoint == NULL) {
        targetAddr += 4;
    }

    // keep each trampoline aligned
    currentTrampolineOff += roundUpToPtrSize(trampolineSize);

    return targetAddr;
}

void setupTrampoline(uint8_t offset) {
#if defined(__i386__)
    trampoline[11] = offset;
#elif defined(__x86_64__)
    trampoline[16] = offset;
#elif defined(__arm__)
    trampoline[8] = offset;
#elif defined(__aarch64__)
    trampoline[9] |= offset << 4;
    trampoline[10] |= offset >> 4;
#else
#error Unsupported architecture
#endif
}

static void *allocTrampolineSpace() {
    unsigned char *buf = mmap(NULL, TRAMPOLINE_SPACE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
                              MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (buf == MAP_FAILED) {
        LOGE("mmap failed, errno = %s", strerror(errno));
        return NULL;
    }
    else {
        LOGI("allocating space for trampoline code at %p", buf);
        return buf;
    }

}
#include "jni.h"
#include <android/log.h>
#include <string.h>
#include <sys/mman.h>

#define LOG_TAG "YAHFA-Native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define DEFAULT_CAP 100

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define roundUpTo4(v) ((v+4-1) - ((v+4-1)&3))

#define ANDROID_L 21
#define ANDROID_L2 22
#define ANDROID_M 23
#define ANDROID_N 24
#define ANDROID_N2 25


static int SDKVersion;

static int OFFSET_dex_cache_resolved_methods_in_ArtMethod;
static int OFFSET_entry_point_from_interpreter_in_ArtMethod;
static int OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod;
static int OFFSET_dex_method_index_in_ArtMethod;
static int OFFSET_hotness_count_in_ArtMethod;
static int OFFSET_array_in_PointerArray;
static int OFFSET_ArtMehod_in_Object;
static size_t pointer_size;
static size_t ArtMethodSize;

static unsigned int hookCap; // capacity for trampolines
static unsigned int hookCount; // current count of used trampolines
static unsigned char *trampolineCode; // place where trampolines are saved
static unsigned int trampolineSize; // trampoline size required for each hook
static unsigned int trampolineCodeSize; // total size of trampoline code area


// trampoline1: set eax to a new ArtMethod addr and then jump into its entry point
#if defined(__i386__)
// b8 78 56 34 12 ; mov eax, 0x12345678
// ff 70 20 ; push dword [eax + 0x20]
// c3 ; ret
static unsigned char trampoline1[] = {0xb8, 0x78, 0x56, 0x34, 0x12, 0xff, 0x70, 0x20, 0xc3};
static unsigned int t1Size = roundUpTo4(sizeof(trampoline1)); // for alignment
#elif defined(__arm__)
// 00 00 9F E5 ; ldr r0, [pc]
// 20 F0 90 E5 ; ldr pc, [r0, 0x20]
// 78 56 34 12 ; 0x12345678
static unsigned char trampoline1[] = {0x00, 0x00, 0x9f, 0xe5, 0x20, 0xf0, 0x90, 0xe5, 0x78, 0x56, 0x34, 0x12};
static unsigned int t1Size = sizeof(trampoline1);
#else
#error Unsupported architecture
#endif

// trampoline2: clear hotness_count of ArtMethod in eax and then jump into its entry point
#if defined(__i386__)
// 66 c7 40 12 00 00 ; mov word [eax + 0x12], 0
// 68 78 56 34 12 ; push 0x12345678
// c3 ; ret
static unsigned char trampoline2[] = {0x66, 0xc7, 0x40, 0x12, 0x00, 0x00, 0x68, 0x78, 0x56, 0x34, 0x12, 0xc3};
static unsigned int t2Size = roundUpTo4(sizeof(trampoline2)); // for alignment
#elif defined(__arm__)
// 04 40 2D E5 ; push {r4}
// 00 40 A0 E3 ; mov r4, #0
// B2 41 C0 E1 ; strh r4, [r0, #18]
// 04 40 9D E4 ; pop {r4}
// 78 56 34 12 ; 0x12345678
static unsigned char trampoline2[] = {
        0x04, 0x40, 0x2d, 0xe5,
        0x00, 0x40, 0xa0, 0xe3,
        0xb2, 0x41, 0xc0, 0xe1,
        0x04, 0x40, 0x9d, 0xe4,
        0x04, 0xF0, 0x1F, 0xE5,
        0x78, 0x56, 0x34, 0x12
};
static unsigned int t2Size = sizeof(trampoline2);
#else
#error Unsupported architecture
#endif

static unsigned int read32(void *addr) {
    return *((unsigned int*)addr);
}

static unsigned long read64(void *addr) {
    return *((unsigned long*)addr);
}

static unsigned long readAddr(void *addr) {
    if(pointer_size == 4) {
        return read32(addr);
    }
    else if(pointer_size == 8) {
        return read64(addr);
    }
    else {
        LOGE("unknown pointer size %d", pointer_size);
        return 0;
    }
}

static int doInitHookCap(unsigned int cap) {
    if(cap == 0) {
        LOGE("invalid capacity: %d", cap);
        return 1;
    }
    if(hookCap) {
        LOGW("allocating new space for trampoline code");
    }
    int allSize = trampolineSize*cap;
    char *buf = mmap(NULL, allSize, PROT_READ, MAP_ANON | MAP_PRIVATE, -1, 0);
    if(buf == MAP_FAILED) {
        LOGE("mmap failed");
        return 1;
    }
    hookCap = cap;
    hookCount = 0;
    trampolineCode = buf;
    trampolineCodeSize = allSize;
    return 0;
}

static void *genTrampoline1(void *hookMethod) {
    void *targetAddr;
    if(mprotect(trampolineCode, trampolineCodeSize, PROT_READ | PROT_WRITE) == -1) {
        LOGE("mprotect RW failed");
        return NULL;
    }
    targetAddr = trampolineCode + trampolineSize*hookCount;
    memcpy(targetAddr, trampoline1, sizeof(trampoline1)); // do not use t1size since it's a rounded size

    // replace with hook ArtMethod addr
#if defined(__i386__)
    memcpy(targetAddr+1, &hookMethod, pointer_size);
#elif defined(__arm__)
    memcpy(targetAddr+8, &hookMethod, pointer_size);
#endif

    if(mprotect(trampolineCode, trampolineCodeSize, PROT_READ | PROT_EXEC) == -1) {
        LOGE("mprotect RX failed");
        return NULL;
    }
    return targetAddr;
}

static void *genTrampoline2(void *entryPoint) {
    void *targetAddr;
    if(mprotect(trampolineCode, trampolineCodeSize, PROT_READ | PROT_WRITE) == -1) {
        LOGE("mprotect RW failed");
        return NULL;
    }
    targetAddr = trampolineCode + trampolineSize*hookCount + t1Size;
    memcpy(targetAddr, trampoline2, sizeof(trampoline2)); // do not use t2size since it's a rounded size

    // replace with real entrypoint
#if defined(__i386__)
    memcpy(targetAddr+7, &entryPoint, pointer_size);
#elif defined(__arm__)
    memcpy(targetAddr+16, &entryPoint, pointer_size);
#endif

    if(mprotect(trampolineCode, trampolineCodeSize, PROT_READ | PROT_EXEC) == -1) {
        LOGE("mprotect RX failed");
        return NULL;
    }
    return targetAddr;
}

void Java_lab_galaxy_yahfa_HookMain_init(JNIEnv *env, jclass clazz, jint sdkVersion) {
    SDKVersion = sdkVersion;
    switch(sdkVersion) {
        case ANDROID_N2:
        case ANDROID_N:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_dex_cache_resolved_methods_in_ArtMethod = 20;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod = 32;
            OFFSET_dex_method_index_in_ArtMethod = 12;
            OFFSET_hotness_count_in_ArtMethod = 18;
            OFFSET_array_in_PointerArray = 0;
            OFFSET_ArtMehod_in_Object = 0;
            pointer_size = sizeof(void *);
            ArtMethodSize = 36;
            trampolineSize = t1Size + t2Size;
            break;
        case ANDROID_M:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_dex_cache_resolved_methods_in_ArtMethod = 4;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod = 36;
            OFFSET_entry_point_from_interpreter_in_ArtMethod = 28;
            OFFSET_dex_method_index_in_ArtMethod = 20;
            OFFSET_array_in_PointerArray = 12;
            OFFSET_ArtMehod_in_Object = 0;
            pointer_size = sizeof(void *);
            ArtMethodSize = 40;
            trampolineSize = t1Size;
            break;
        case ANDROID_L2:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_dex_cache_resolved_methods_in_ArtMethod = 12;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod = 44;
            OFFSET_entry_point_from_interpreter_in_ArtMethod = 36;
            OFFSET_dex_method_index_in_ArtMethod = 28;
            OFFSET_array_in_PointerArray = 12;
            OFFSET_ArtMehod_in_Object = 8;
            pointer_size = sizeof(void *);
            ArtMethodSize = 48;
            trampolineSize = t1Size;
            break;
        case ANDROID_L:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_dex_cache_resolved_methods_in_ArtMethod = 12;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod = 40;
            OFFSET_entry_point_from_interpreter_in_ArtMethod = 24;
            OFFSET_dex_method_index_in_ArtMethod = 64;
            OFFSET_array_in_PointerArray = 12;
            OFFSET_ArtMehod_in_Object = 8;
            pointer_size = sizeof(void *);
            ArtMethodSize = 72;
            trampolineSize = t1Size;
            break;
        default:
            LOGE("not compatible with SDK %d", sdkVersion);
            break;
    }
#if defined(__i386__)
    trampoline1[7] = OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod;
#elif defined(__arm__)
    trampoline1[4] = OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod;
#endif
}

static int doBackupAndHook(void *originMethod, void *hookMethod, void *backupMethod) {
    if(hookCount >= hookCap) {
        LOGW("not enough capacity");
        if(doInitHookCap(DEFAULT_CAP)) {
            LOGE("cannot hook method");
            return 1;
        }
    }

//    LOGI("origin method is at %p, hook method is at %p", originMethod, hookMethod);
    if(!backupMethod) {
        LOGW("backup method is null");
    }
    else { //do method backup
        void *dexCacheResolvedMethods = (void *)readAddr((void *)((char *)backupMethod+OFFSET_dex_cache_resolved_methods_in_ArtMethod));
        int methodIndex = read32((void *)((char *)backupMethod+
                OFFSET_dex_method_index_in_ArtMethod));
        //first update the cached method manually
        memcpy((char *)dexCacheResolvedMethods+OFFSET_array_in_PointerArray+pointer_size*methodIndex,
               (&backupMethod),
               pointer_size);
        //then replace the placeholder with original method
        memcpy((void *)((char *)backupMethod+OFFSET_ArtMehod_in_Object),
                (void *)((char *)originMethod+OFFSET_ArtMehod_in_Object),
                ArtMethodSize-OFFSET_ArtMehod_in_Object);

        // if Android N, then use trampoline2 to reset hotness_count
        if(SDKVersion >= ANDROID_N) {
            void *realEntryPoint = readAddr((char *) originMethod +
                                            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod);
            void *newEntryPoint = genTrampoline2(realEntryPoint);
            if(newEntryPoint) {
                memcpy((char *) backupMethod + OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod,
                       &newEntryPoint, pointer_size);
            }
            else {
                LOGE("failed to allocate space for backup method trampoline");
                return 1;
            }
        }
//        LOGI("backup method is at %p", backupMethod);
    }

    // replace entry point
    void *newEntrypoint = genTrampoline1(hookMethod);
//    LOGI("origin ep is %p, new ep is %p",
//         readAddr((char *) originMethod + OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod),
//         newEntrypoint
//    );
    if(newEntrypoint) {
        memcpy((char *) originMethod + OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod,
               &newEntrypoint,
               pointer_size);
    }
    else {
        LOGW("failed to allocate space for trampoline");
        return 1;
    }

    if(OFFSET_entry_point_from_interpreter_in_ArtMethod != 0) {
        memcpy((char *) originMethod + OFFSET_entry_point_from_interpreter_in_ArtMethod,
               (char *) hookMethod + OFFSET_entry_point_from_interpreter_in_ArtMethod,
               pointer_size);
    }

    LOGI("hook and backup done");
    hookCount += 1;
    return 0;
}

void Java_lab_galaxy_yahfa_HookMain_findAndBackupAndHook(JNIEnv *env, jclass clazz,
    jclass targetClass, jstring methodName, jstring methodSig, jobject hook, jobject backup) {
    if(!methodName || !methodSig) {
        LOGE("empty method name or signature");
        return;
    }
    const char *c_methodName = (*env)->GetStringUTFChars(env, methodName, NULL);
    const char *c_methodSig = (*env)->GetStringUTFChars(env, methodSig, NULL);
    if(c_methodName == NULL || c_methodSig == NULL) {
        LOGE("failed to get c string");
        return;
    }
    void *targetMethod = NULL;
    LOGI("Start findAndBackupAndHook for method %s%s", c_methodName, c_methodSig);
    if(pointer_size == 0) {
        LOGE("Not initialized");
        goto end;
    }
    targetMethod = (void *)(*env)->GetMethodID(env, targetClass, c_methodName, c_methodSig);
    if((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env); //cannot find non-static method
        targetMethod = (void *)(*env)->GetStaticMethodID(env, targetClass, c_methodName, c_methodSig);
        if((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env); //cannot find static method
            LOGE("Cannot find target method %s%s", c_methodName, c_methodSig);
            goto end;
        }
    }
    if(!doBackupAndHook(targetMethod, (void *)(*env)->FromReflectedMethod(env, hook),
        backup==NULL ? NULL : (void *)(*env)->FromReflectedMethod(env, backup))) {
        (*env)->NewGlobalRef(env, hook); // keep a global ref so that the hook method would not be GCed
    }
end:
    (*env)->ReleaseStringUTFChars(env, methodName, c_methodName);
    (*env)->ReleaseStringUTFChars(env, methodSig, c_methodSig);
}

#include "jni.h"
#include <string.h>
#include <sys/mman.h>

#include "common.h"
#include "env.h"
#include "trampoline.h"

static int SDKVersion;
static int OFFSET_entry_point_from_interpreter_in_ArtMethod;
static int OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod;
static int OFFSET_hotness_count_in_ArtMethod;
static int OFFSET_ArtMehod_in_Object;
static int ArtMethodSize;

static inline uint32_t read32(void *addr) {
    return *((uint32_t *)addr);
}

static inline uint64_t read64(void *addr) {
    return *((uint64_t *)addr);
}

void Java_lab_galaxy_yahfa_HookMain_init(JNIEnv *env, jclass clazz, jint sdkVersion) {
    int i;
    SDKVersion = sdkVersion;
    switch(sdkVersion) {
        case ANDROID_N2:
        case ANDROID_N:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_ArtMehod_in_Object = 0;
            OFFSET_hotness_count_in_ArtMethod = 4*4+2 ; // sizeof(GcRoot<mirror::Class>) = 4

            // ptr_sized_fields_ is rounded up to pointer_size in ArtMethod
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    roundUpToPtrSize(4*4+2*2) + pointer_size*3;

            ArtMethodSize = roundUpToPtrSize(4*4+2*2)+pointer_size*4;
            break;
        case ANDROID_M:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_ArtMehod_in_Object = 0;
            OFFSET_entry_point_from_interpreter_in_ArtMethod = roundUpToPtrSize(4*7);
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    OFFSET_entry_point_from_interpreter_in_ArtMethod + pointer_size*2;
            ArtMethodSize = roundUpToPtrSize(4*7)+pointer_size*3;
            break;
        case ANDROID_L2:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_ArtMehod_in_Object = 4*2;
            OFFSET_entry_point_from_interpreter_in_ArtMethod = roundUpToPtrSize(OFFSET_ArtMehod_in_Object+4*7);
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    OFFSET_entry_point_from_interpreter_in_ArtMethod+pointer_size*2;
            ArtMethodSize = OFFSET_entry_point_from_interpreter_in_ArtMethod+pointer_size*3;
            break;
        case ANDROID_L:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_ArtMehod_in_Object = 4*2;
            OFFSET_entry_point_from_interpreter_in_ArtMethod = OFFSET_ArtMehod_in_Object+4*4;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    OFFSET_entry_point_from_interpreter_in_ArtMethod+8*2;
            ArtMethodSize = OFFSET_ArtMehod_in_Object+4*4+8*4+4*4;
            break;
        default:
            LOGE("not compatible with SDK %d", sdkVersion);
            break;
    }
#if defined(__i386__)
    trampoline1[7] = OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod;
    if(SDKVersion < ANDROID_N) { // do not set hotness_count before N
        memset(trampoline2+5, '\x90', 6);
    }
#elif defined(__arm__)
    trampoline1[4] = (unsigned char)OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod;
    if(SDKVersion < ANDROID_N) { // do not set hotness_count before N
        for(i=4; i<=16; i+=4) {
            memcpy(trampoline2+i, "\x00\x00\xa0\xe1", 4); // mov r0, r0
        }
    }
#elif defined(__aarch64__)
    if(SDKVersion < ANDROID_N) { // do not set hotness_count before N
        memcpy(trampoline2+4, "\x1f\x20\x03\xd5", 4); // nop
        if(SDKVersion == ANDROID_L2) {
            memcpy(trampoline1+4, "\x10\x1c\x40\xf9", 4); //101c40f9 ; ldr x16, [x0, #56]
        }
        else if(SDKVersion == ANDROID_L) {
            memcpy(trampoline1+4, "\x10\x14\x40\xf9", 4); //101440f9 ; ldr x16, [x0, #40]
        }
    }
#endif
}

static int doBackupAndHook(void *originMethod, void *hookMethod, void *backupMethod) {
    if(hookCount >= hookCap) {
        LOGW("not enough capacity. Allocating...");
        if(doInitHookCap(DEFAULT_CAP)) {
            LOGE("cannot hook method");
            return 1;
        }
        LOGI("Allocating done");
    }

//    LOGI("origin method is at %p, hook method is at %p, backup method is at %p",
//         originMethod, hookMethod, backupMethod);

    if(!backupMethod) {
        LOGW("backup method is null");
    }
    else { //do method backup
        void *realEntryPoint = (void *)readAddr((char *) originMethod +
                                        OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod);
        void *newEntryPoint = genTrampoline2(originMethod, realEntryPoint);
        if(newEntryPoint) {
            memcpy((char *) backupMethod + OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod,
                   &newEntryPoint, pointer_size);
        }
        else {
            LOGE("failed to allocate space for backup method trampoline");
            return 1;
        }
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
    if(ArtMethodSize == 0) {
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

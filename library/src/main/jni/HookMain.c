#include "jni.h"
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>

#include "common.h"
#include "env.h"
#include "trampoline.h"

int SDKVersion;
static int OFFSET_entry_point_from_interpreter_in_ArtMethod;
int OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod;
int OFFSET_hotness_count_in_ArtMethod;
static int OFFSET_dex_method_index_in_ArtMethod;
static int OFFSET_dex_cache_resolved_methods_in_ArtMethod;
static int OFFSET_array_in_PointerArray;
static int OFFSET_ArtMehod_in_Object;
static int ArtMethodSize;

static inline uint16_t read16(void *addr) {
    return *((uint16_t *)addr);
}

static inline uint32_t read32(void *addr) {
    return *((uint32_t *)addr);
}

static inline uint64_t read64(void *addr) {
    return *((uint64_t *)addr);
}

void Java_lab_galaxy_yahfa_HookMain_init(JNIEnv *env, jclass clazz, jint sdkVersion) {
    int i;
    SDKVersion = sdkVersion;
    LOGI("init to SDK %d", sdkVersion);
    switch(sdkVersion) {
        case ANDROID_O:
            OFFSET_ArtMehod_in_Object = 0;
            OFFSET_hotness_count_in_ArtMethod = 4*4+2;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    roundUpToPtrSize(4*4+2*2) + pointer_size*2;
            ArtMethodSize = roundUpToPtrSize(4*4+2*2)+pointer_size*3;
            break;
        case ANDROID_N2:
        case ANDROID_N:
            OFFSET_ArtMehod_in_Object = 0;
            OFFSET_hotness_count_in_ArtMethod = 4*4+2; // sizeof(GcRoot<mirror::Class>) = 4
            OFFSET_dex_method_index_in_ArtMethod = 4*3;
            OFFSET_dex_cache_resolved_methods_in_ArtMethod = roundUpToPtrSize(4*4+2*2);
            OFFSET_array_in_PointerArray = 0;

            // ptr_sized_fields_ is rounded up to pointer_size in ArtMethod
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    roundUpToPtrSize(4*4+2*2) + pointer_size*3;

            ArtMethodSize = roundUpToPtrSize(4*4+2*2)+pointer_size*4;
            break;
        case ANDROID_M:
            OFFSET_ArtMehod_in_Object = 0;
            OFFSET_entry_point_from_interpreter_in_ArtMethod = roundUpToPtrSize(4*7);
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    OFFSET_entry_point_from_interpreter_in_ArtMethod + pointer_size*2;
            OFFSET_dex_method_index_in_ArtMethod = 4*5;
            OFFSET_dex_cache_resolved_methods_in_ArtMethod = 4;
            OFFSET_array_in_PointerArray = 4*3;
            ArtMethodSize = roundUpToPtrSize(4*7)+pointer_size*3;
            break;
        case ANDROID_L2:
            OFFSET_ArtMehod_in_Object = 4*2;
            OFFSET_entry_point_from_interpreter_in_ArtMethod = roundUpToPtrSize(OFFSET_ArtMehod_in_Object+4*7);
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    OFFSET_entry_point_from_interpreter_in_ArtMethod+pointer_size*2;
            ArtMethodSize = OFFSET_entry_point_from_interpreter_in_ArtMethod+pointer_size*3;
            break;
        case ANDROID_L:
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

    /*
#if defined(__i386__)
    trampoline1[18] = OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod;
    if(SDKVersion < ANDROID_N) { // do not set hotness_count before N
        memset(trampoline1, '\x90', 11);
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
            memcpy(trampoline1+4, "\x10\x1c\x40\xf9", 4); //101c40f9 ; ldr x16, [x0, #56] set entry point offset
        }
        else if(SDKVersion == ANDROID_L) {
            memcpy(trampoline1+4, "\x10\x14\x40\xf9", 4); //101440f9 ; ldr x16, [x0, #40] set entry point offset
        }
    }
#endif
     */
}

static int doBackupAndHook(void *targetMethod, void *hookMethod, void *backupMethod) {
    if(hookCount >= hookCap) {
        LOGW("not enough capacity. Allocating...");
        if(doInitHookCap(DEFAULT_CAP)) {
            LOGE("cannot hook method");
            return 1;
        }
        LOGI("Allocating done");
    }

    LOGI("target method is at %p, hook method is at %p, backup method is at %p",
         targetMethod, hookMethod, backupMethod);

    if(!backupMethod) {
        LOGW("Origin method is null. Cannot call origin");
    }
    else { //do method backup
        //first update the cached method manually
        void *dexCacheResolvedMethods = (void *) readAddr((void *) ((char *) hookMethod +
                                                                    OFFSET_dex_cache_resolved_methods_in_ArtMethod));
        int methodIndex = read32((void *) ((char *) backupMethod + OFFSET_dex_method_index_in_ArtMethod));
        memcpy((char *) dexCacheResolvedMethods + OFFSET_array_in_PointerArray +
               pointer_size * methodIndex,
               (&backupMethod),
               pointer_size);

        // have to copy the whole target ArtMethod here
        // if the target method calls other methods which are to be resolved
        // then ToDexPC would be invoked for the caller(origin method)
        // in which case ToDexPC would use the entrypoint as a base for mapping pc to dex offset
        // so any changes to the target method's entrypoint would result in a wrong dex offset
        // and artQuickResolutionTrampoline would fail for methods called by the origin method
        memcpy(backupMethod, targetMethod, ArtMethodSize);
    }

    // replace entry point
    void *newEntrypoint = genTrampoline1(hookMethod, backupMethod);
    LOGI("origin ep is %p, new ep is %p",
         readAddr((char *) targetMethod + OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod),
         newEntrypoint
    );
    if(newEntrypoint) {
        memcpy((char *) targetMethod + OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod,
               &newEntrypoint,
               pointer_size);
    }
    else {
        LOGW("failed to allocate space for trampoline");
        return 1;
    }

    if(OFFSET_entry_point_from_interpreter_in_ArtMethod != 0) {
        memcpy((char *) targetMethod + OFFSET_entry_point_from_interpreter_in_ArtMethod,
               (char *) hookMethod + OFFSET_entry_point_from_interpreter_in_ArtMethod,
               pointer_size);
    }

    LOGI("hook and backup done");
    hookCount += 1;
    return 0;
}

void Java_lab_galaxy_yahfa_HookMain_findAndBackupAndHook(JNIEnv *env, jclass clazz,
    jclass targetClass, jstring methodName, jstring methodSig, jboolean isStatic,
    jobject hook, jobject backup) {
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
    LOGI("Start findAndBackupAndHook for %s method %s%s", isStatic ? "static" : "non-static", c_methodName, c_methodSig);
    if(ArtMethodSize == 0) {
        LOGE("Not initialized");
        goto end;
    }
    if(!isStatic) { // non-static
        targetMethod = (void *) (*env)->GetMethodID(env, targetClass, c_methodName, c_methodSig);
    }
    else {// static
        targetMethod = (void *)(*env)->GetStaticMethodID(env, targetClass, c_methodName, c_methodSig);
    }

    if((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        LOGE("Cannot find target %s method: %s%s", isStatic ? "static" : "non-static", c_methodName, c_methodSig);
        goto end;
    }

    if(!doBackupAndHook(
            targetMethod,
            (void *)(*env)->FromReflectedMethod(env, hook),
            backup==NULL ? NULL : (void *)(*env)->FromReflectedMethod(env, backup)
    )) {
        (*env)->NewGlobalRef(env, hook); // keep a global ref so that the hook method would not be GCed
    }
end:
    (*env)->ReleaseStringUTFChars(env, methodName, c_methodName);
    (*env)->ReleaseStringUTFChars(env, methodSig, c_methodSig);
}

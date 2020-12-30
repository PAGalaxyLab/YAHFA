#include <jni.h>
#include <android/log.h>
#include <stdint.h>

#include "common.h"
#include <dlfcn.h>
#include "dlfunc.h"

static char *classLinker = NULL;
typedef void (*InitClassFunc)(void *, void *, int);
static InitClassFunc MakeInitializedClassesVisiblyInitialized = NULL;
static int shouldVisiblyInit();
static int findInitClassSymbols(JNIEnv *env);

static int findInitClassSymbols(JNIEnv *env) {
    int OFFSET_classlinker_in_Runtime;
    if(SDKVersion == __ANDROID_API_R__) {
#if defined(__x86_64__) || defined(__aarch64__)
        OFFSET_classlinker_in_Runtime = 472;
#else
        OFFSET_classlinker_in_Runtime = 276;
#endif
    }
    if(dlfunc_init(env) != JNI_OK) {
        LOGE("dlfunc init failed");
        return 1;
    }
    void *handle = dlfunc_dlopen(env, "libart.so", RTLD_LAZY);
    if(handle == NULL) {
        LOGE("failed to find libart.so handle");
        return 1;
    }
    else {
        void *runtime_bss = dlfunc_dlsym(env, handle, "_ZN3art7Runtime9instance_E");
        if(!runtime_bss) {
            LOGE("failed to find Runtime::instance symbol");
            return 1;
        }
        char *runtime = readAddr(runtime_bss);
        if(!runtime) {
            LOGE("Runtime::instance value is NULL");
            return 1;
        }
        LOGI("runtime bss is at %p, runtime instance is at %p", runtime_bss, runtime);
        classLinker = readAddr(runtime + OFFSET_classlinker_in_Runtime);
        LOGI("classLinker is at %p, value %p", runtime + OFFSET_classlinker_in_Runtime, classLinker);

        MakeInitializedClassesVisiblyInitialized = dlfunc_dlsym(env, handle,
                "_ZN3art11ClassLinker40MakeInitializedClassesVisiblyInitializedEPNS_6ThreadEb");
//        "_ZN3art11ClassLinker12AllocIfTableEPNS_6ThreadEm"); // for test
        if(!MakeInitializedClassesVisiblyInitialized) {
            LOGE("failed to find MakeInitializedClassesVisiblyInitialized symbol");
            return 1;
        }
        LOGI("MakeInitializedClassesVisiblyInitialized is at %p",
             MakeInitializedClassesVisiblyInitialized);
    }
    return 0;
}

jlong __attribute__((naked)) Java_lab_galaxy_yahfa_HookMain_00024Utils_getThread(JNIEnv *env, jclass clazz) {
#if defined(__aarch64__)
    __asm__(
            "mov x0, x19\n"
            "ret\n"
            );
#elif defined(__arm__)
    __asm__(
            "mov r0, r9\n"
            "bx lr\n"
            );
#elif defined(__x86_64__)
    __asm__(
            "mov %gs:0xe8, %rax\n" // offset on Android R
            "ret\n"
            );
#elif defined(__i386__)
    __asm__(
            "mov %fs:0xc4, %eax\n" // offset on Android R
            "ret\n"
            );
#endif
}

static int shouldVisiblyInit() {
#if defined(__i386__) || defined(__x86_64__)
    return 1;
#else
    if(SDKVersion < __ANDROID_API_R__) {
        return 0;
    }
    else return 1;
#endif
}

jboolean Java_lab_galaxy_yahfa_HookMain_00024Utils_shouldVisiblyInit(JNIEnv *env, jclass clazz) {
    return shouldVisiblyInit() != 0;
}

jint Java_lab_galaxy_yahfa_HookMain_00024Utils_visiblyInit(JNIEnv *env, jclass clazz, jlong thread) {
    if(!shouldVisiblyInit()) {
        return 0;
    }

    if(!classLinker || !MakeInitializedClassesVisiblyInitialized) {
        if(findInitClassSymbols(env) != 0) {
            LOGE("failed to find symbols: classLinker %p, MakeInitializedClassesVisiblyInitialized %p",
                 classLinker, MakeInitializedClassesVisiblyInitialized);
            return 1;
        }
    }

    LOGI("thread is at %p", thread);
    MakeInitializedClassesVisiblyInitialized(classLinker, (void *)thread, 1);
    return 0;
}
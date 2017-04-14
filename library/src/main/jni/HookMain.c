#include "jni.h"
#include <android/log.h>
#include <string.h>

#define LOG_TAG "YAHFA-Native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

void hook_new_entry_24();
void hook_new_entry_23();
void hook_new_entry_22();
void hook_new_entry_21();

static int OFFSET_dex_cache_resolved_methods_in_ArtMethod;
static int OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod;
static int OFFSET_entry_point_from_jni_in_ArtMethod;
static int OFFSET_dex_method_index_in_ArtMethod;
static int OFFSET_array_in_PointerArray;
static int OFFSET_ArtMehod_in_Object;
static size_t pointer_size;
static size_t ArtMethodSize;
static void *hook_new_entry;

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

void Java_lab_galaxy_yahfa_HookMain_init(JNIEnv *env, jclass clazz, jint sdkVersion) {
    switch(sdkVersion) {
        case 25:
        case 24:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_dex_cache_resolved_methods_in_ArtMethod = 20;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod = 32;
            OFFSET_entry_point_from_jni_in_ArtMethod = 28;
            OFFSET_dex_method_index_in_ArtMethod = 12;
            OFFSET_array_in_PointerArray = 0;
            OFFSET_ArtMehod_in_Object = 0;
            pointer_size = sizeof(void *);
            ArtMethodSize = 36;
            hook_new_entry = (void *)hook_new_entry_24;
            break;
        case 23:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_dex_cache_resolved_methods_in_ArtMethod = 4;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod = 36;
            OFFSET_entry_point_from_jni_in_ArtMethod = 32;
            OFFSET_dex_method_index_in_ArtMethod = 20;
            OFFSET_array_in_PointerArray = 12;
            OFFSET_ArtMehod_in_Object = 0;
            pointer_size = sizeof(void *);
            ArtMethodSize = 40;
            hook_new_entry = (void *)hook_new_entry_23;
            break;
        case 22:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_dex_cache_resolved_methods_in_ArtMethod = 12;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod = 44;
            OFFSET_entry_point_from_jni_in_ArtMethod = 40;
            OFFSET_dex_method_index_in_ArtMethod = 28;
            OFFSET_array_in_PointerArray = 12;
            OFFSET_ArtMehod_in_Object = 8;
            pointer_size = sizeof(void *);
            ArtMethodSize = 48;
            hook_new_entry = (void *)hook_new_entry_22;
            break;
        case 21:
            LOGI("init to SDK %d", sdkVersion);
            OFFSET_dex_cache_resolved_methods_in_ArtMethod = 12;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod = 40;
            OFFSET_entry_point_from_jni_in_ArtMethod = 32;
            OFFSET_dex_method_index_in_ArtMethod = 64;
            OFFSET_array_in_PointerArray = 12;
            OFFSET_ArtMehod_in_Object = 8;
            pointer_size = sizeof(void *);
            ArtMethodSize = 64;
            hook_new_entry = (void *)hook_new_entry_21;
            break;
        default:
            LOGE("not compatible with SDK %d", sdkVersion);
            break;
    }
}

static void doBackupAndHook(void *originMethod, void *hookMethod, void *backupMethod) {
    if(!backupMethod) {
        LOGW("backup method is null");
    }
    else { //do method backup
        void *dexCacheResolvedMethods = (void *)readAddr((void *)((char *)backupMethod+OFFSET_dex_cache_resolved_methods_in_ArtMethod));
        int methodIndex = read32((void *)((char *)backupMethod+OFFSET_dex_method_index_in_ArtMethod));
        //first update the cached method manually
        memcpy((void *)((char *)dexCacheResolvedMethods+OFFSET_array_in_PointerArray+pointer_size*methodIndex), (void *)(&backupMethod), pointer_size);
        //then replace the placeholder with original method
        memcpy((void *)((char *)backupMethod+OFFSET_ArtMehod_in_Object), 
                (void *)((char *)originMethod+OFFSET_ArtMehod_in_Object), 
                ArtMethodSize-OFFSET_ArtMehod_in_Object);
    }
    //do method replacement
    //save the hook method at entry_point_from_jni_
    memcpy((void *)((char *)originMethod+OFFSET_entry_point_from_jni_in_ArtMethod), (void *)(&hookMethod), pointer_size);
    //update the entrypoint
    memcpy((void *)((char *)originMethod+OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod), (void *)(&hook_new_entry), pointer_size);
    LOGI("hook and backup done");
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
    doBackupAndHook(targetMethod, (void *)(*env)->FromReflectedMethod(env, hook), 
        backup==NULL ? NULL : (void *)(*env)->FromReflectedMethod(env, backup));

end:
    (*env)->ReleaseStringUTFChars(env, methodName, c_methodName);
    (*env)->ReleaseStringUTFChars(env, methodSig, c_methodSig);
}

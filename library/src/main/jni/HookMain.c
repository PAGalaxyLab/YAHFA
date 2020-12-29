#include <jni.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "common.h"
#include "trampoline.h"
#include "dlfunc.h"

static int SDKVersion;
static uint32_t OFFSET_entry_point_from_interpreter_in_ArtMethod;
static uint32_t OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod;
static uint32_t OFFSET_ArtMehod_in_Object;
static uint32_t OFFSET_access_flags_in_ArtMethod;
static uint32_t kAccNative = 0x0100;
static uint32_t kAccCompileDontBother = 0x01000000;
static uint32_t kAccFastInterpreterToInterpreterInvoke = 0x40000000;

static jfieldID fieldArtMethod = NULL;

static uint32_t OFFSET_classlinker_in_Runtime;
static char *classLinker = NULL;
typedef void (*InitClassFunc)(void *, int);
static InitClassFunc MakeInitializedClassesVisiblyInitialized = NULL;
static int shouldVisiblyInit();
static int findInitClassSymbols(JNIEnv *env);

static inline uint32_t read32(void *addr) {
    return *((uint32_t *) addr);
}

static inline void write32(void *addr, uint32_t value) {
    *((uint32_t *) addr) = value;
}

static inline void *readAddr(void *addr) {
    return *((void **) addr);
}

static inline void writeAddr(void *addr, void *value) {
    *((void **)addr) = value;
}

static int findInitClassSymbols(JNIEnv *env) {
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
        void **runtime_bss = dlfunc_dlsym(env, handle, "_ZN3art7Runtime9instance_E");
        if(!runtime_bss) {
            LOGE("failed to find Runtime::instance symbol");
            return 1;
        }
        char *runtime = *runtime_bss;
        if(!runtime) {
            LOGE("Runtime::instance value is NULL");
            return 1;
        }
        classLinker = runtime + OFFSET_classlinker_in_Runtime;

        MakeInitializedClassesVisiblyInitialized =
                dlfunc_dlsym(env, handle, "_ZN3art11ClassLinker40MakeInitializedClassesVisiblyInitializedEPNS_6ThreadEb");
        if(!MakeInitializedClassesVisiblyInitialized) {
            LOGE("failed to find MakeInitializedClassesVisiblyInitialized symbol");
            return 1;
        }
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
    return 0;
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

    MakeInitializedClassesVisiblyInitialized((void *)thread, 1);
    return 0;
}

void Java_lab_galaxy_yahfa_HookMain_init(JNIEnv *env, jclass clazz, jint sdkVersion) {
    SDKVersion = sdkVersion;
    jclass classExecutable;
    LOGI("init to SDK %d", sdkVersion);
    switch (sdkVersion) {
        case __ANDROID_API_R__:
            classExecutable = (*env)->FindClass(env, "java/lang/reflect/Executable");
            fieldArtMethod = (*env)->GetFieldID(env, classExecutable, "artMethod", "J");
#if defined(__x86_64__) || defined(__aarch64__)
            OFFSET_classlinker_in_Runtime = 472;
#else
            OFFSET_classlinker_in_Runtime = 276;
#endif
        case __ANDROID_API_Q__:
        case __ANDROID_API_P__:
            kAccCompileDontBother = 0x02000000;
            OFFSET_ArtMehod_in_Object = 0;
            OFFSET_access_flags_in_ArtMethod = 4;
            //OFFSET_dex_method_index_in_ArtMethod = 4*3;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    roundUpToPtrSize(4 * 4 + 2 * 2) + pointer_size;
            break;
        case __ANDROID_API_O_MR1__:
            kAccCompileDontBother = 0x02000000;
        case __ANDROID_API_O__:
            OFFSET_ArtMehod_in_Object = 0;
            OFFSET_access_flags_in_ArtMethod = 4;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    roundUpToPtrSize(4 * 4 + 2 * 2) + pointer_size * 2;
            break;
        case __ANDROID_API_N_MR1__:
        case __ANDROID_API_N__:
            OFFSET_ArtMehod_in_Object = 0;
            OFFSET_access_flags_in_ArtMethod = 4; // sizeof(GcRoot<mirror::Class>) = 4
            // ptr_sized_fields_ is rounded up to pointer_size in ArtMethod
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    roundUpToPtrSize(4 * 4 + 2 * 2) + pointer_size * 3;
            break;
        case __ANDROID_API_M__:
            OFFSET_ArtMehod_in_Object = 0;
            OFFSET_entry_point_from_interpreter_in_ArtMethod = roundUpToPtrSize(4 * 7);
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    OFFSET_entry_point_from_interpreter_in_ArtMethod + pointer_size * 2;
            break;
        case __ANDROID_API_L_MR1__:
            OFFSET_ArtMehod_in_Object = 4 * 2;
            OFFSET_entry_point_from_interpreter_in_ArtMethod = roundUpToPtrSize(
                    OFFSET_ArtMehod_in_Object + 4 * 7);
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    OFFSET_entry_point_from_interpreter_in_ArtMethod + pointer_size * 2;
            break;
        case __ANDROID_API_L__:
            OFFSET_ArtMehod_in_Object = 4 * 2;
            OFFSET_entry_point_from_interpreter_in_ArtMethod = OFFSET_ArtMehod_in_Object + 4 * 4;
            OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod =
                    OFFSET_entry_point_from_interpreter_in_ArtMethod + 8 * 2;
            break;
        default:
            LOGE("not compatible with SDK %d", sdkVersion);
            break;
    }

    setupTrampoline(OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod);
}

static uint32_t getFlags(char *method) {
    uint32_t access_flags = read32(method + OFFSET_access_flags_in_ArtMethod);
    return access_flags;
}

static void setFlags(char *method, uint32_t access_flags) {
    write32(method + OFFSET_access_flags_in_ArtMethod, access_flags);
}

static void setNonCompilable(void *method) {
    if (SDKVersion < __ANDROID_API_N__) {
        return;
    }
    uint32_t access_flags = getFlags(method);
    uint32_t old_flags = access_flags;
    access_flags |= kAccCompileDontBother;
    setFlags(method, access_flags);
    LOGI("setNonCompilable: change access flags from 0x%x to 0x%x", old_flags, access_flags);

}

static int replaceMethod(void *fromMethod, void *toMethod, int isBackup) {
    LOGI("replace method from %p to %p", fromMethod, toMethod);

    // replace entry point
    void *newEntrypoint = NULL;
    if(isBackup) {
        void *originEntrypoint = readAddr((char *) toMethod + OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod);
        // entry point hardcoded
        newEntrypoint = genTrampoline(toMethod, originEntrypoint);
    }
    else {
        // entry point from ArtMethod struct
        newEntrypoint = genTrampoline(toMethod, NULL);
    }

    LOGI("replace entry point from %p to %p",
         readAddr((char *) fromMethod + OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod),
         newEntrypoint
    );
    if (newEntrypoint) {
        writeAddr((char *) fromMethod + OFFSET_entry_point_from_quick_compiled_code_in_ArtMethod,
                newEntrypoint);
    } else {
        LOGE("failed to allocate space for trampoline of target method");
        return 1;
    }

    if (OFFSET_entry_point_from_interpreter_in_ArtMethod != 0) {
        void *interpEntrypoint = readAddr((char *) toMethod + OFFSET_entry_point_from_interpreter_in_ArtMethod);
        writeAddr((char *) fromMethod + OFFSET_entry_point_from_interpreter_in_ArtMethod,
                interpEntrypoint);
    }

    // set the target method to native so that Android O wouldn't invoke it with interpreter
    if(SDKVersion >= __ANDROID_API_O__) {
        uint32_t access_flags = getFlags(fromMethod);
        uint32_t old_flags = access_flags;
        if (SDKVersion >= __ANDROID_API_Q__) {
            // On API 29 whether to use the fast path or not is cached in the ART method structure
            access_flags &= ~kAccFastInterpreterToInterpreterInvoke;
        }
        if (SDKVersion <= __ANDROID_API_Q__) {
            // We don't set kAccNative on R+ because they will try to load from real native method pointer instead of entry_point_from_quick_compiled_code_.
            // Ref: https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:art/runtime/art_method.h;l=844;bpv=1;bpt=1
            access_flags |= kAccNative;
        }
        setFlags(fromMethod, access_flags);
        LOGI("change access flags from 0x%x to 0x%x", old_flags, access_flags);
    }

    return 0;

}

static int doBackupAndHook(void *targetMethod, void *hookMethod, void *backupMethod) {
    LOGI("target method is at %p, hook method is at %p, backup method is at %p",
         targetMethod, hookMethod, backupMethod);

    int res = 0;

    // set kAccCompileDontBother for a method we do not want the compiler to compile
    // so that we don't need to worry about hotness_count_
    if (SDKVersion >= __ANDROID_API_N__) {
        setNonCompilable(targetMethod);
//        setNonCompilable(hookMethod);
        if(backupMethod) setNonCompilable(backupMethod);
    }

    if (backupMethod) {// do method backup
        // we use the same way as hooking target method
        // hook backup method and redirect back to the original target method
        // the only difference is that the entry point is now hardcoded
        // instead of reading from ArtMethod struct since it's overwritten
        res += replaceMethod(backupMethod, targetMethod, 1);
    }

    res += replaceMethod(targetMethod, hookMethod, 0);

    LOGI("hook and backup done");
    return res;
}

static void *getArtMethod(JNIEnv *env, jobject jmethod) {
    void *artMethod = NULL;

    if(jmethod == NULL) {
        return artMethod;
    }

    if(SDKVersion == __ANDROID_API_R__) {
        artMethod = (void *) (*env)->GetLongField(env, jmethod, fieldArtMethod);
    }
    else {
        artMethod = (void *) (*env)->FromReflectedMethod(env, jmethod);
    }

    LOGI("ArtMethod: %p", artMethod);
    return artMethod;

}

jobject Java_lab_galaxy_yahfa_HookMain_findMethodNative(JNIEnv *env, jclass clazz,
                                                        jclass targetClass, jstring methodName,
                                                        jstring methodSig) {
    const char *c_methodName = (*env)->GetStringUTFChars(env, methodName, NULL);
    const char *c_methodSig = (*env)->GetStringUTFChars(env, methodSig, NULL);
    jobject ret = NULL;


    //Try both GetMethodID and GetStaticMethodID -- Whatever works :)
    jmethodID method = (*env)->GetMethodID(env, targetClass, c_methodName, c_methodSig);
    if (!(*env)->ExceptionCheck(env)) {
        ret = (*env)->ToReflectedMethod(env, targetClass, method, JNI_FALSE);
    } else {
        (*env)->ExceptionClear(env);
        method = (*env)->GetStaticMethodID(env, targetClass, c_methodName, c_methodSig);
        if (!(*env)->ExceptionCheck(env)) {
            ret = (*env)->ToReflectedMethod(env, targetClass, method, JNI_TRUE);
        } else {
            (*env)->ExceptionClear(env);
        }
    }

    (*env)->ReleaseStringUTFChars(env, methodName, c_methodName);
    (*env)->ReleaseStringUTFChars(env, methodSig, c_methodSig);
    return ret;
}

jboolean Java_lab_galaxy_yahfa_HookMain_backupAndHookNative(JNIEnv *env, jclass clazz,
                                                            jobject target, jobject hook,
                                                            jobject backup) {



    if (!doBackupAndHook(
            getArtMethod(env, target),
            getArtMethod(env, hook),
            getArtMethod(env, backup)
    )) {
        (*env)->NewGlobalRef(env, hook); // keep a global ref so that the hook method would not be GCed
        if(backup) (*env)->NewGlobalRef(env, backup);
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

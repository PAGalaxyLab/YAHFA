#include <jni.h>
#include <android/log.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include "common.h"

#if defined(__aarch64__)
#include <dlfcn.h>
#include "dlfunc.h"
#define NEED_CLASS_VISIBLY_INITIALIZED
#endif

static char *classLinker = NULL;
typedef void (*InitClassFunc)(void *, void *, int);
static InitClassFunc MakeInitializedClassesVisiblyInitialized = NULL;
static int shouldVisiblyInit();
static int findInitClassSymbols(JNIEnv *env);

static const size_t kPointerSize = sizeof(void *);
static const size_t kVTablePosition = 2;

static int isValidAddress(const void *p) {
    if (!p) {
        return 0;
    }

    int ret = 1;
    int fd = open("/dev/random", O_WRONLY);
    size_t len = sizeof(int32_t);
    if (fd != -1) {
        if (write(fd, p, len) < 0) {
            ret = 0;
        }
        close(fd);
    } else {
        ret = 0;
    }
    return ret;
}

static int
commonFindOffset(void *start, size_t max_count, size_t step, void *value, int start_search_offset) {
    if (NULL == start) {
        return -1;
    }
    if (start_search_offset > max_count) {
        return -1;
    }

    for (int i = start_search_offset; i <= max_count; i += step) {
        void *current_value = *(void **) ((size_t) start + i);
        if (value == current_value) {
            return i;
        }
    }
    return -1;
}

static int searchClassLinkerOffset(JavaVM *vm, void *runtime_instance, JNIEnv *env, void *libart_handle) {
#ifndef NEED_CLASS_VISIBLY_INITIALIZED
    return -1;
#else
    void *class_linker_vtable = dlfunc_dlsym(env, libart_handle, "_ZTVN3art11ClassLinkerE");
    if (class_linker_vtable != NULL) {
        // Before Android 9, class_liner do not hava virtual table, so class_linker_vtable is null.
        class_linker_vtable = (char *) class_linker_vtable + kPointerSize * kVTablePosition;
    }

    //Need to search jvm offset from 200, if not, on xiaomi android7.1.2 we would get jvm_offset 184, but in fact it is 440
    int jvm_offset_in_runtime = commonFindOffset(runtime_instance, 2000, 4, (void *) vm, 200);

    if (jvm_offset_in_runtime < 0) {
        LOGE("failed to find JavaVM in Runtime");
        return -1;
    }

    int step = 4;
    int class_linker_offset_value = -1;
    for (int i = jvm_offset_in_runtime - step; i > 0; i -= step) {
        void *class_linker_addr = *(void **) ((uintptr_t) runtime_instance + i);

        if (class_linker_addr == NULL || !isValidAddress(class_linker_addr)) {
            continue;
        }
        if (class_linker_vtable != NULL) {
            if (*(void **) class_linker_addr == class_linker_vtable) {
                class_linker_offset_value = i;
                break;
            }
        } else {
            // in runtime.h, the struct is:
            //    ThreadList* thread_list_;
            //    InternTable* intern_table_;
            //    ClassLinker* class_linker_;
            // these objects list as this kind of order.
            // And the InternTable pointer is also saved in ClassLinker struct,
            // So, we can search ClassLinker struct to verify the intern_table_ address.
            void *intern_table_addr = *(void **) (
                    (uintptr_t) runtime_instance +
                    i -
                    kPointerSize);
            if (isValidAddress(intern_table_addr)) {
                for (int j = 200; j < 500; j += step) {
                    void *intern_table = *(void **) (
                            (uintptr_t) class_linker_addr + j);
                    if (intern_table_addr == intern_table) {
                        class_linker_offset_value = i;
                        break;
                    }
                }
            }
        }
        if (class_linker_offset_value > 0) {
            break;
        }
    }
    return class_linker_offset_value;
#endif
}

static int findInitClassSymbols(JNIEnv *env) {
#ifndef NEED_CLASS_VISIBLY_INITIALIZED
    return 1;
#else
    JavaVM *jvm = NULL;
    (*env)->GetJavaVM(env, &jvm);
    LOGI("JavaVM is %p", jvm);

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

        int class_linker_offset_in_Runtime = searchClassLinkerOffset(jvm, runtime, env, handle);
        LOGI("find class_linker offset in_Runtime --> %d ", class_linker_offset_in_Runtime);

        if(class_linker_offset_in_Runtime < 0) {
            LOGE("failed to find class_linker offset in Runtime");
            return 1;
        }

        classLinker = readAddr(runtime + class_linker_offset_in_Runtime);
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
#endif
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

    LOGI("thread is at %p", thread);
    MakeInitializedClassesVisiblyInitialized(classLinker, (void *)thread, 1);
    return 0;
}

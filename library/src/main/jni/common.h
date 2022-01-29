//
// Created by liuruikai756 on 05/07/2017.
//
#ifndef YAHFA_COMMON_H
#define YAHFA_COMMON_H

#include <jni.h>
#include <android/log.h>
#include <stdint.h>

// Android 12+
#ifndef __ANDROID_API_S_L__
#define __ANDROID_API_S_L__ 32
#endif

#ifndef __ANDROID_API_S__
#define __ANDROID_API_S__ 31
#endif

extern int SDKVersion;

#define LOG_TAG "YAHFA-Native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define pointer_size sizeof(void*)
#define roundUpToPtrSize(v) (v + pointer_size - 1 - ((v + pointer_size - 1) & (pointer_size - 1)))

#define read32(addr) *((uint32_t *)(addr))

#define write32(addr, value) *((uint32_t *)(addr)) = (value)

#define readAddr(addr) *((void **)(addr))

#define writeAddr(addr, value) *((void **)(addr)) = (value)

#endif //YAHFA_COMMON_H

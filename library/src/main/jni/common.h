//
// Created by liuruikai756 on 05/07/2017.
//
#include <android/log.h>

#ifndef YAHFA_COMMON_H
#define YAHFA_COMMON_H

#define LOG_TAG "YAHFA-Native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define pointer_size sizeof(void*)
#define roundUpToPtrSize(v) (v + pointer_size - 1 - ((v + pointer_size - 1) & (pointer_size - 1)))

#endif //YAHFA_COMMON_H

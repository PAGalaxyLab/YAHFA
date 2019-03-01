//
// Created by liuruikai756 on 05/07/2017.
//

#ifndef YAHFA_ENV_H
#define YAHFA_ENV_H

#define ANDROID_L 21
#define ANDROID_L2 22
#define ANDROID_M 23
#define ANDROID_N 24
#define ANDROID_N2 25
#define ANDROID_O 26
#define ANDROID_O2 27
#define ANDROID_P 28

#define pointer_size sizeof(void*)
#define roundUpToPtrSize(v) (v + pointer_size - 1 - (v - 1) % pointer_size)

#endif //YAHFA_ENV_H

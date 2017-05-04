//
// Created by lrk on 5/4/17.
//
#include <jni.h>
#include <android/log.h>

jstring Java_lab_galaxy_yahfa_demoApp_ClassWithJNIMethod_fromJNI(JNIEnv *env, jclass clazz) {
    return (*env)->NewStringUTF(env, "hello from JNI");
}
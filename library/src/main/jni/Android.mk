LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= HookMain.c trampoline.c

LOCAL_LDLIBS    := -llog

LOCAL_MODULE:= yahfa

include $(BUILD_SHARED_LIBRARY)

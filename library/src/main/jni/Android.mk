LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq "$(TARGET_ARCH_ABI)" "x86"
	LOCAL_SRC_FILES:= HookMain.c entrypoints_x86.S
else
	LOCAL_SRC_FILES:= HookMain.c entrypoints_arm.S
endif

LOCAL_LDLIBS    := -llog

LOCAL_MODULE:= yahfa

include $(BUILD_SHARED_LIBRARY)

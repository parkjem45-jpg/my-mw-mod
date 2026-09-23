LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := mw_mod
LOCAL_SRC_FILES := \
    main.cpp \
    menu.cpp \
    hooks.cpp \
    imgui/imgui.cpp \
    imgui/imgui_draw.cpp \
    imgui/imgui_tables.cpp \
    imgui/imgui_widgets.cpp \
    imgui/backends/imgui_impl_opengl3.cpp \
    imgui/backends/imgui_impl_android.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/imgui \
    $(LOCAL_PATH)/imgui/backends

LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2 -lGLESv3 -L$(LOCAL_PATH) -ldobby

LOCAL_CPPFLAGS := -std=c++17 -fno-exceptions -fno-rtti -DIMGUI_IMPL_OPENGL_ES3

include $(BUILD_SHARED_LIBRARY)

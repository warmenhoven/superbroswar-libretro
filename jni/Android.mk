LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..
LIBRETRO_DIR := $(CORE_DIR)/libretro

DEBUG                    := 0
FRONTEND_SUPPORTS_RGB565 := 1
NEED_BPP                 := 16
NEED_STEREO_SOUND        := 1
HAVE_CHD                 := 0
IS_X86                   := 0
FLAGS                    :=

ifeq ($(TARGET_ARCH),x86)
  IS_X86 := 1
endif

include $(CORE_DIR)/Makefile.common

COREFLAGS := $(FLAGS) -funroll-loops $(INCFLAGS) -D__LIBRETRO__ $(CORE_DEFINE)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE       := retro
LOCAL_SRC_FILES    := $(SOURCES_C) $(SOURCES_CXX)
LOCAL_CFLAGS       := $(COREFLAGS)
LOCAL_CONLYFLAGS   := $(C_WARNINGS)
LOCAL_CXXFLAGS     := $(CXXFLAGS) $(COREFLAGS) -Wno-c++11-narrowing
LOCAL_LDFLAGS      := -Wl,-version-script=$(CORE_DIR)/libretro/link.T
include $(BUILD_SHARED_LIBRARY)
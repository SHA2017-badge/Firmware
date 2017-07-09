PROJECT_NAME := sha2017-badge-firmware

ifndef PROJECT_PATH
PROJECT_PATH := $(abspath $(dir $(firstword $(MAKEFILE_LIST))))
export PROJECT_PATH
endif

UGFX_PATH := $(PROJECT_PATH)/../ugfx
export UGFX_PATH

EXTRA_COMPONENT_DIRS := $(PROJECT_PATH)/../components

include $(IDF_PATH)/make/project.mk

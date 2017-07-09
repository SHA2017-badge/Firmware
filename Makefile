#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := sha2017-badge-test

ifndef PROJECT_PATH
PROJECT_PATH := $(abspath $(dir $(firstword $(MAKEFILE_LIST))))
export PROJECT_PATH
endif

UGFX_PATH := $(PROJECT_PATH)/ugfx
export UGFX_PATH

include $(IDF_PATH)/make/project.mk

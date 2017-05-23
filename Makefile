#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

#Magic hackery to allow shell envs to be set in Makefile
IGNORE := $(shell bash -c "source set_env.sh; env | sed 's/=/:=/' | sed 's/^/export /' > makeenv")
include makeenv

PROJECT_NAME := sha2017-badge-test

include $(IDF_PATH)/make/project.mk



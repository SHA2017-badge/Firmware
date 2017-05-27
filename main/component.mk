#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name).a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#
COMPONENT_EXTRA_INCLUDES = $(PROJECT_PATH)/components/ugfx $(PROJECT_PATH)/ugfx $(PROJECT_PATH)/ugfx/src $(PROJECT_PATH)/ugfx/drivers/gdisp/framebuffer $(PROJECT_PATH)/esp-idf/components/freertos/include/freertos

# Component Makefile

COMPONENT_ADD_INCLUDEDIRS := .

COMPONENT_EXTRA_INCLUDES := \
	$(UGFX_PATH) \
	$(UGFX_PATH)/drivers/gdisp/framebuffer \
	$(IDF_PATH)/components/freertos/include/freertos \


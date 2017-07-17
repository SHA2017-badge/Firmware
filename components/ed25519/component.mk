#
# Component Makefile
#


COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_SRCDIRS := .
COMPONENT_PRIV_INCLUDEDIRS := .

CFLAGS :=  $(filter-out -O1 -O2 -Og,$(CFLAGS)) -O3
# Include the main makefile fragment to build the MicroPython component.
SRC_QSTR += $(wildcard ../mods/*.c)

include $(MICROPYTHON_TOP)/ports/embed/embed.mk

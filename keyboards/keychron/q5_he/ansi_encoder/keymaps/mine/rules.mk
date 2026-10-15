USER_NAME := simon

# Raw HID interface 1 (32 bytes): VIA / Keychron Launcher and SignalRGB.
# Interface 2 (64 bytes, usage page 0xFF61): kbd-daemon's lighting (OpenRGB
# packets) and host protocol.
VIA_ENABLE = yes
RAW_ENABLE = yes
RGB_RAW_ENABLE = yes
OPENRGB_ENABLE = yes
SIGNALRGB_ENABLE = yes
OPT_DEFS += -DOPENRGB_ENABLE -DSIGNALRGB_ENABLE

# The STM32F401 has three endpoints; XInput's goes to the second raw HID
# interface. Gamepad mode stays as the HID joystick on the shared endpoint.
OPT_DEFS := $(filter-out -DXINPUT_ENABLE,$(OPT_DEFS))

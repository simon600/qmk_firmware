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

# Raw HID over 2.4 GHz / Bluetooth: the module relays the 32-byte interface
# (0xFF60 on the Keychron Link) and its reports reach via_raw_hid_receive.
# Keychron's spelling.
OPT_DEFS += -DWILRESS_RAW_ENABLE

# The STM32F401 has three endpoints; XInput's goes to the second raw HID
# interface. Gamepad mode stays as the HID joystick on the shared endpoint.
OPT_DEFS := $(filter-out -DXINPUT_ENABLE,$(OPT_DEFS))

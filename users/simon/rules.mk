SRC += simon.c
CAPS_WORD_ENABLE = yes
LEADER_ENABLE = yes
DYNAMIC_MACRO_ENABLE = yes

ifeq ($(OPENRGB_ENABLE), yes)
    SRC += openrgb.c
    RGB_MATRIX_CUSTOM_USER = yes
    # Host protocol (kbd-daemon): on the OpenRGB packets' interface, the second
    # raw HID one with RGB_RAW_ENABLE (see host_raw.h)
    SRC += host_protocol.c
    OPT_DEFS += -DHOST_PROTOCOL_ENABLE
endif


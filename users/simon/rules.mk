SRC += simon.c
CAPS_WORD_ENABLE = yes
LEADER_ENABLE = yes
DYNAMIC_MACRO_ENABLE = yes

ifeq ($(OPENRGB_ENABLE), yes)
    SRC += openrgb.c
    RGB_MATRIX_CUSTOM_USER = yes
    # Host protocol (kbd-daemon) rides on the same raw HID interface
    SRC += host_protocol.c
    OPT_DEFS += -DHOST_PROTOCOL_ENABLE
endif


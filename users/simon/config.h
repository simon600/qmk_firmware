#pragma once

#define COMBO_TERM 25
#define LEADER_PER_KEY_TIMING
#define LEADER_TIMEOUT 250
#define TAPPING_TERM_PER_KEY

// Host protocol keyboards keep settings + indicator colours in a user datablock.
// It sits before VIA's EEPROM area and shifts it: turning it on for a keyboard
// that already has VIA (Q1 v2) would scramble its saved dynamic keymap once.
#ifdef HOST_PROTOCOL_ENABLE
#    define EECONFIG_USER_DATA_SIZE 72
#    define EECONFIG_USER_DATA_VERSION 0x53494D05 // "SIM" + layout 5 (layer dim)
#endif

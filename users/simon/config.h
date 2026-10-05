#pragma once

#define COMBO_TERM 25
#define LEADER_PER_KEY_TIMING
#define LEADER_TIMEOUT 250
#define TAPPING_TERM_PER_KEY

// Host protocol keyboards keep settings + indicator colours in a user datablock.
// Never on VIA keyboards: the datablock shifts the dynamic keymap in EEPROM.
#ifdef HOST_PROTOCOL_ENABLE
#    define EECONFIG_USER_DATA_SIZE 64
#    define EECONFIG_USER_DATA_VERSION 0x53494D02 // "SIM" + layout 2 (fn-layer colour)
#endif

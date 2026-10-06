#pragma once

#define COMBO_TERM 25
#define LEADER_PER_KEY_TIMING
#define LEADER_TIMEOUT 250
#define TAPPING_TERM_PER_KEY

// Host protocol keyboards keep settings + indicator colours in a user datablock.
// Never on VIA keyboards: the datablock shifts the dynamic keymap in EEPROM.
#ifdef HOST_PROTOCOL_ENABLE
#    define EECONFIG_USER_DATA_SIZE 72
#    define EECONFIG_USER_DATA_VERSION 0x53494D05 // "SIM" + layout 5 (layer dim)
#endif

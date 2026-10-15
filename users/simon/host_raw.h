#pragma once

#include "raw_hid.h"

// The protocols kbd-daemon speaks (OpenRGB 0x01–0x09, host protocol 0xC0–0xCF)
// have their own 64-byte raw HID interface with RGB_RAW_ENABLE; otherwise they
// share the raw HID interface with everything else. Replies go back where the
// command came from.
#ifdef RGB_RAW_ENABLE
#    define HOST_PACKET_LEN RGB_RAW_EPSIZE
#    define host_packet_send(packet) rgb_raw_hid_send(packet, RGB_RAW_EPSIZE)
#else
#    define HOST_PACKET_LEN RAW_EPSIZE
#    define host_packet_send(packet) raw_hid_send(packet, RAW_EPSIZE)
#endif

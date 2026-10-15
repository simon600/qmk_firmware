// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>

#define RAW_HID_SRC_USB 0

/**
 * \file
 *
 * \defgroup raw_hid Raw HID API
 * \{
 */

/**
 * \brief Callback, invoked when a raw HID report has been received from the host.
 *
 * \param data A pointer to the received data. Always 32 bytes in length.
 * \param length The length of the buffer. Always 32.
 */
void raw_hid_receive(uint8_t src, uint8_t *data, uint8_t length);

/**
 * \brief Send an HID report.
 *
 * \param data A pointer to the data to send. Must always be 32 bytes in length.
 * \param length The length of the buffer. Must always be 32.
 */
void raw_hid_send(uint8_t *data, uint8_t length);

#ifdef RGB_RAW_ENABLE
/* Second raw HID interface, USB only: 64-byte reports on usage page
 * RGB_RAW_USAGE_PAGE, beside the VIA one. Replies to its commands must go
 * out through rgb_raw_hid_send(); raw_hid_send() answers on the VIA one. */
#    define RGB_RAW_EPSIZE 64

/**
 * \brief Callback, invoked when a report arrives on the second raw HID interface.
 *
 * \param data A pointer to the received data, RGB_RAW_EPSIZE bytes.
 * \param length The length of the buffer, RGB_RAW_EPSIZE.
 */
void rgb_raw_hid_receive(uint8_t *data, uint8_t length);

/**
 * \brief Sends a report on the second raw HID interface. Dropped unless
 * length is RGB_RAW_EPSIZE or while USB isn't configured.
 */
void rgb_raw_hid_send(uint8_t *data, uint8_t length);
#endif

/** \} */

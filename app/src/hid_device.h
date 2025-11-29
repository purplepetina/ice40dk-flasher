/**
 * @file hid_device.h
 * @brief HID device configuration and interface for ICE40 Flasher
 */

#ifndef HID_DEVICE_H
#define HID_DEVICE_H

#include <zephyr/usb/class/usbd_hid.h>

/**
 * @brief Initialize and register the HID device
 * 
 * @return 0 on success, negative errno on failure
 */
int hid_device_init(void);

#endif /* HID_DEVICE_H */

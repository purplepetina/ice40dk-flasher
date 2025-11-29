/**
 * @file usbd_init.h
 * @brief USB Device initialization and configuration interface
 */

#ifndef USBD_INIT_H
#define USBD_INIT_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Initialize the USB device with descriptors and configurations
 *
 * Sets up the USB device with manufacturer/product descriptors,
 * configures both Full-Speed and High-Speed (if supported),
 * and registers all required USB classes.
 *
 * @param msg_cb Optional message callback for USB events (can be NULL)
 * @return Pointer to the initialized USB context, or NULL on failure
 */
struct usbd_context *flasher_usbd_init_device(usbd_msg_cb_t msg_cb);

#endif /* USBD_INIT_H */


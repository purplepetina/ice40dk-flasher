#ifndef USBD_INIT_H
#define USBD_INIT_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Initialize the USB device with default descriptors and configurations.
 *
 * @param msg_cb Optional message callback.
 * @return struct usbd_context* Pointer to the initialized USB context, or NULL on failure.
 */
struct usbd_context *flasher_usbd_init_device(usbd_msg_cb_t msg_cb);

#endif /* USBD_INIT_H */

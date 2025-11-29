/**
 * @file usbd_init.c
 * @brief USB Device initialization and configuration
 */

#include "usbd_init.h"

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/bos.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(usbd_config);

/* USB Device Identification */
#define ZEPHYR_PROJECT_USB_VID  0x2FE3
#define ZEPHYR_PROJECT_USB_PID  0x1193

/* Device strings */
#define MANUFACTURER_STRING     "Purple Petina"
#define PRODUCT_STRING          "ICE40DK Programmer"

/* Configuration attributes and power */
#define USB_MAX_POWER_MA        100

/**
 * @brief USB classes to exclude from registration
 * 
 * By default, do not register the DFU mode instance
 */
static const char *const class_blocklist[] = {
	"dfu_dfu",
	NULL,
};

/* Instantiate USB device context */
USBD_DEVICE_DEFINE(flasher_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   ZEPHYR_PROJECT_USB_VID,
		   ZEPHYR_PROJECT_USB_PID);

/* USB Descriptor definitions */
USBD_DESC_LANG_DEFINE(lang_desc);
USBD_DESC_MANUFACTURER_DEFINE(mfr_desc, MANUFACTURER_STRING);
USBD_DESC_PRODUCT_DEFINE(product_desc, PRODUCT_STRING);

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");

/* Configuration attributes */
static const uint8_t config_attributes =
	(IS_ENABLED(CONFIG_FLASHER_USBD_SELF_POWERED) ? USB_SCD_SELF_POWERED : 0) |
	(IS_ENABLED(CONFIG_FLASHER_USBD_REMOTE_WAKEUP) ? USB_SCD_REMOTE_WAKEUP : 0);

/* USB Configuration definitions */
USBD_CONFIGURATION_DEFINE(fs_config, config_attributes,
			  USB_MAX_POWER_MA, &fs_cfg_desc);

USBD_CONFIGURATION_DEFINE(hs_config, config_attributes,
			  USB_MAX_POWER_MA, &hs_cfg_desc);

/**
 * @brief Set appropriate device class code triple
 *
 * Configures the device class, subclass, and protocol based on
 * enabled USB classes. For multi-interface classes, use the
 * IAD (Interface Association Descriptor) triple.
 *
 * @param uds_ctx USB device context
 * @param speed USB speed (FS or HS)
 */
static void configure_device_class(struct usbd_context *uds_ctx,
				    const enum usbd_speed speed)
{
	/* Use class code from Interface Descriptors for most classes */
	if (IS_ENABLED(CONFIG_USBD_CDC_ACM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_CDC_ECM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_CDC_NCM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_MIDI2_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_VIDEO_CLASS)) {
		/*
		 * Multi-interface classes have Interface Association
		 * Descriptor - use Miscellaneous Device Class triple
		 */
		usbd_device_set_code_triple(uds_ctx, speed,
					    USB_BCC_MISCELLANEOUS, 0x02, 0x01);
	} else {
		/* Use interface-defined class codes */
		usbd_device_set_code_triple(uds_ctx, speed, 0, 0, 0);
	}
}

/**
 * @brief Add descriptors and register USB classes
 *
 * @param msg_cb Optional message callback for USB events
 * @return Pointer to configured USB context, or NULL on failure
 */
static struct usbd_context *setup_usb_device(usbd_msg_cb_t msg_cb)
{
	int err;

	/* Add language descriptor */
	err = usbd_add_descriptor(&flasher_usbd, &lang_desc);
	if (err) {
		LOG_ERR("Failed to add language descriptor: %d", err);
		return NULL;
	}

	/* Add manufacturer descriptor */
	err = usbd_add_descriptor(&flasher_usbd, &mfr_desc);
	if (err) {
		LOG_ERR("Failed to add manufacturer descriptor: %d", err);
		return NULL;
	}

	/* Add product descriptor */
	err = usbd_add_descriptor(&flasher_usbd, &product_desc);
	if (err) {
		LOG_ERR("Failed to add product descriptor: %d", err);
		return NULL;
	}

	/* Configure High-Speed if supported */
	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(&flasher_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&flasher_usbd, USBD_SPEED_HS,
					     &hs_config);
		if (err) {
			LOG_ERR("Failed to add HS configuration: %d", err);
			return NULL;
		}

		err = usbd_register_all_classes(&flasher_usbd, USBD_SPEED_HS,
						1, class_blocklist);
		if (err) {
			LOG_ERR("Failed to register HS classes: %d", err);
			return NULL;
		}

		configure_device_class(&flasher_usbd, USBD_SPEED_HS);
	}

	/* Configure Full-Speed (always supported) */
	err = usbd_add_configuration(&flasher_usbd, USBD_SPEED_FS, &fs_config);
	if (err) {
		LOG_ERR("Failed to add FS configuration: %d", err);
		return NULL;
	}

	err = usbd_register_all_classes(&flasher_usbd, USBD_SPEED_FS,
					1, class_blocklist);
	if (err) {
		LOG_ERR("Failed to register FS classes: %d", err);
		return NULL;
	}

	configure_device_class(&flasher_usbd, USBD_SPEED_FS);

	/* Configure power mode */
	usbd_self_powered(&flasher_usbd,
			  config_attributes & USB_SCD_SELF_POWERED);

	/* Register optional message callback */
	if (msg_cb != NULL) {
		err = usbd_msg_register_cb(&flasher_usbd, msg_cb);
		if (err) {
			LOG_ERR("Failed to register message callback: %d", err);
			return NULL;
		}
	}

	return &flasher_usbd;
}

struct usbd_context *flasher_usbd_init_device(usbd_msg_cb_t msg_cb)
{
	int err;

	if (setup_usb_device(msg_cb) == NULL) {
		return NULL;
	}

	err = usbd_init(&flasher_usbd);
	if (err) {
		LOG_ERR("Failed to initialize USB device: %d", err);
		return NULL;
	}

	LOG_INF("USB device initialized successfully");
	return &flasher_usbd;
}


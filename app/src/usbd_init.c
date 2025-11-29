#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/bos.h>
#include <zephyr/usb/usbd.h>

#include "usbd_init.h"

LOG_MODULE_REGISTER(usbd_config);

#define ZEPHYR_PROJECT_USB_VID 0x2fe3
#define ZEPHYR_PROJECT_USB_PID 0x1193

/* By default, do not register the USB DFU class DFU mode instance. */
static const char *const blocklist[] = {
    "dfu_dfu",
    NULL,
};

/* Instantiate a context named flasher_usbd using the default USB device
 * controller, the Zephyr project vendor ID, and the sample product ID.
 */
USBD_DEVICE_DEFINE(flasher_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
                   ZEPHYR_PROJECT_USB_VID, ZEPHYR_PROJECT_USB_PID);

USBD_DESC_LANG_DEFINE(lang_desc);
USBD_DESC_MANUFACTURER_DEFINE(mfr_desc, "Purple Petina");
USBD_DESC_PRODUCT_DEFINE(product_desc, "ICE40DK Programmer");
/* IF_ENABLED(CONFIG_HWINFO, (USBD_DESC_SERIAL_NUMBER_DEFINE(1141))); */

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");

static const uint8_t attributes =
    (IS_ENABLED(CONFIG_FLASHER_USBD_SELF_POWERED) ? USB_SCD_SELF_POWERED : 0) |
    (IS_ENABLED(CONFIG_FLASHER_USBD_REMOTE_WAKEUP) ? USB_SCD_REMOTE_WAKEUP : 0);

/* Full speed configuration */
USBD_CONFIGURATION_DEFINE(fs_config, attributes,
                          100, &fs_cfg_desc);

/* High speed configuration */
USBD_CONFIGURATION_DEFINE(hs_config, attributes,
                          100, &hs_cfg_desc);

static void fix_code_triple(struct usbd_context *uds_ctx,
                                   const enum usbd_speed speed) {
  /* Always use class code information from Interface Descriptors */
  if (IS_ENABLED(CONFIG_USBD_CDC_ACM_CLASS) ||
      IS_ENABLED(CONFIG_USBD_CDC_ECM_CLASS) ||
      IS_ENABLED(CONFIG_USBD_CDC_NCM_CLASS) ||
      IS_ENABLED(CONFIG_USBD_MIDI2_CLASS) ||
      IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) ||
      IS_ENABLED(CONFIG_USBD_VIDEO_CLASS)) {
    /*
     * Class with multiple interfaces have an Interface
     * Association Descriptor available, use an appropriate triple
     * to indicate it.
     */
    usbd_device_set_code_triple(uds_ctx, speed, USB_BCC_MISCELLANEOUS, 0x02,
                                0x01);
  } else {
    usbd_device_set_code_triple(uds_ctx, speed, 0, 0, 0);
  }
}

static struct usbd_context *usbd_setup_device(usbd_msg_cb_t msg_cb) {
  int err;

  err = usbd_add_descriptor(&flasher_usbd, &lang_desc);
  if (err) {
    LOG_ERR("Failed to initialize language descriptor (%d)", err);
    return NULL;
  }

  err = usbd_add_descriptor(&flasher_usbd, &mfr_desc);
  if (err) {
    LOG_ERR("Failed to initialize manufacturer descriptor (%d)", err);
    return NULL;
  }

  err = usbd_add_descriptor(&flasher_usbd, &product_desc);
  if (err) {
    LOG_ERR("Failed to initialize product descriptor (%d)", err);
    return NULL;
  }

  /*
  if (IS_ENABLED(CONFIG_HWINFO)) {
      err = usbd_add_descriptor(&flasher_usbd, &sample_sn);
      if (err) {
          LOG_ERR("Failed to initialize SN descriptor (%d)", err);
          return NULL;
      }
  }
  */

  if (USBD_SUPPORTS_HIGH_SPEED &&
      usbd_caps_speed(&flasher_usbd) == USBD_SPEED_HS) {
    err =
        usbd_add_configuration(&flasher_usbd, USBD_SPEED_HS, &hs_config);
    if (err) {
      LOG_ERR("Failed to add High-Speed configuration");
      return NULL;
    }

    err = usbd_register_all_classes(&flasher_usbd, USBD_SPEED_HS, 1, blocklist);
    if (err) {
      LOG_ERR("Failed to add register classes");
      return NULL;
    }

    fix_code_triple(&flasher_usbd, USBD_SPEED_HS);
  }

  err = usbd_add_configuration(&flasher_usbd, USBD_SPEED_FS, &fs_config);
  if (err) {
    LOG_ERR("Failed to add Full-Speed configuration");
    return NULL;
  }

  err = usbd_register_all_classes(&flasher_usbd, USBD_SPEED_FS, 1, blocklist);
  if (err) {
    LOG_ERR("Failed to add register classes");
    return NULL;
  }

  fix_code_triple(&flasher_usbd, USBD_SPEED_FS);
  usbd_self_powered(&flasher_usbd, attributes & USB_SCD_SELF_POWERED);

  if (msg_cb != NULL) {
    err = usbd_msg_register_cb(&flasher_usbd, msg_cb);
    if (err) {
      LOG_ERR("Failed to register message callback");
      return NULL;
    }
  }

  return &flasher_usbd;
}

struct usbd_context *flasher_usbd_init_device(usbd_msg_cb_t msg_cb) {
  int err;

  if (usbd_setup_device(msg_cb) == NULL) {
    return NULL;
  }

  err = usbd_init(&flasher_usbd);
  if (err) {
    LOG_ERR("Failed to initialize device support");
    return NULL;
  }

  return &flasher_usbd;
}

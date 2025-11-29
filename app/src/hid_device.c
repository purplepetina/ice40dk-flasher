#include "hid_device.h"

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/class/usbd_hid.h>

LOG_MODULE_REGISTER(hid_device);

#define REPORT_SIZE_BYTES 64

/* HID Report Descriptor for vendor-defined interface */
static const uint8_t hid_report_desc[] = {
    0x06, 0x00, 0xFF, /* USAGE_PAGE (Vendor Defined 0xFF00) */
    0x09, 0x01,       /* USAGE (Vendor Usage 1) */
    0xA1, 0x01,       /* COLLECTION (Application) */

    /* OUT report: host -> device */
    0x09, 0x02,              /*   USAGE (Vendor Usage 2) */
    0x15, 0x00,              /*   LOGICAL_MINIMUM (0) */
    0x26, 0xFF, 0x00,        /*   LOGICAL_MAXIMUM (255) */
    0x75, 0x08,              /*   REPORT_SIZE (8 bits) */
    0x95, REPORT_SIZE_BYTES, /*   REPORT_COUNT (64 bytes) */
    0x91, 0x02,              /*   OUTPUT (Data,Var,Abs) */

    /* IN report: device -> host */
    0x09, 0x03,              /*   USAGE (Vendor Usage 3) */
    0x15, 0x00,              /*   LOGICAL_MINIMUM (0) */
    0x26, 0xFF, 0x00,        /*   LOGICAL_MAXIMUM (255) */
    0x75, 0x08,              /*   REPORT_SIZE (8 bits) */
    0x95, REPORT_SIZE_BYTES, /*   REPORT_COUNT (64 bytes) */
    0x81, 0x02,              /*   INPUT (Data,Var,Abs) */

    0xC0 /* END_COLLECTION */
};

/**
 * @brief Callback when HID interface becomes ready or not ready
 */
static void hid_iface_ready(const struct device *dev, const bool ready) {
  LOG_INF("HID device %s interface is %s", dev->name,
          ready ? "ready" : "not ready");
}

/**
 * @brief Handle GET_REPORT requests from host
 */
static int hid_get_report(const struct device *dev, const uint8_t type,
                          const uint8_t id, const uint16_t len,
                          uint8_t *const buf) {
  LOG_DBG("Get Report: Type %u ID %u Len %u", type, id, len);
  return 0;
}

/**
 * @brief Handle SET_REPORT requests from host
 */
static int hid_set_report(const struct device *dev, const uint8_t type,
                          const uint8_t id, const uint16_t len,
                          const uint8_t *const buf) {
  LOG_INF("Set Report: Type %u ID %u Len %u", type, id, len);
  LOG_HEXDUMP_INF(buf, len, "HID OUT data:");
  return 0;
}

static const struct hid_device_ops hid_ops = {
    .iface_ready = hid_iface_ready,
    .get_report = hid_get_report,
    .set_report = hid_set_report,
};

int hid_device_init(void) {
  const struct device *hid_dev = DEVICE_DT_GET(DT_NODELABEL(hid_dev_0));
  int err;

  if (!device_is_ready(hid_dev)) {
    LOG_ERR("HID device not ready");
    return -ENODEV;
  }

  err = hid_device_register(hid_dev, hid_report_desc, sizeof(hid_report_desc),
                            &hid_ops);
  if (err) {
    LOG_ERR("Failed to register HID device: %d", err);
    return err;
  }

  LOG_INF("HID device initialized successfully");
  return 0;
}

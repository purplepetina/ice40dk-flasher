#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/input/input.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/usb/usbd.h>

#include "usbd_init.h"
#include "w25q16_hal.h"

#define RESET_PIN DT_NODELABEL(crst)
#define FLASH_NODE DT_NODELABEL(w25q16)
#define SPI_OP SPI_OP_MODE_MASTER | SPI_WORD_SET(8)

#if !DT_NODE_HAS_STATUS(FLASH_NODE, okay)
#error "w25q16 is not defined..."
#endif

LOG_MODULE_REGISTER(main);

static struct gpio_dt_spec crst = GPIO_DT_SPEC_GET(RESET_PIN, gpios);
static const uint8_t hid_report_desc[] = {
    0x06, 0x00, 0xFF, // USAGE_PAGE (Vendor Defined 0xFF00)
    0x09, 0x01,       // USAGE (Vendor Usage 1)
    0xA1, 0x01,       // COLLECTION (Application)

    // OUT report: host -> device
    0x09, 0x02,       //   USAGE (Vendor Usage 2)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //   REPORT_SIZE (8 bits)
    0x95, 0x40,       //   REPORT_COUNT (64 bytes)
    0x91, 0x02,       //   OUTPUT (Data,Var,Abs)

    // IN report: device -> host
    0x09, 0x03,       //   USAGE (Vendor Usage 3)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //   REPORT_SIZE (8 bits)
    0x95, 0x40,       //   REPORT_COUNT (64 bytes)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)

    0xC0 // END_COLLECTION
};

static void hid_iface_ready(const struct device *dev, const bool ready) {
  LOG_INF("HID device %s interface is %s", dev->name,
          ready ? "ready" : "not ready");
}

static int hid_get_report(const struct device *dev, const uint8_t type,
                            const uint8_t id, const uint16_t len,
                            uint8_t *const buf) {
  LOG_WRN("Get Report not implemented, Type %u ID %u", type, id);

  return 0;
}

static int hid_set_report(const struct device *dev, const uint8_t type,
                            const uint8_t id, const uint16_t len,
                            const uint8_t *const buf) {
  LOG_INF("Set Report: Type %u ID %u Len %u", type, id, len);
  LOG_HEXDUMP_INF(buf, len, "Data");
  return 0;
}

static const struct hid_device_ops hid_ops = {
    .iface_ready = hid_iface_ready,
    .get_report = hid_get_report,
    .set_report = hid_set_report,
};

int main() {

  int err;

  const struct device *hid_dev = DEVICE_DT_GET(DT_NODELABEL(hid_dev_0));

  if (!device_is_ready(hid_dev)) {
    LOG_ERR("Failed to initialize hid device");
    return 1;
  }

  err = hid_device_register(hid_dev, hid_report_desc, sizeof(hid_report_desc),
                            &hid_ops);
  if (err) {
    LOG_ERR("Failed to register HID Device");
    return 1;
  }

  struct usbd_context *usbd_ctx = flasher_usbd_init_device(NULL);
  if (!usbd_ctx) {
    LOG_ERR("Failed to initialize USB device");
    return 1;
  }

  err = usbd_enable(usbd_ctx);
  if (err) {
    LOG_ERR("Failed to enable usbd");
    return 1;
  }

  struct flash_config flash_dev = {
      .dev = SPI_DT_SPEC_GET(FLASH_NODE, SPI_OP, 0),
  };

  err = gpio_pin_configure_dt(&crst, GPIO_OUTPUT_ACTIVE);
  if (err < 0) {
    LOG_ERR("Failed to init reset in");
    return 1;
  }

  gpio_pin_set_dt(&crst, 1);
  k_msleep(2);

  flash_reset(&flash_dev);
  flash_read_id(&flash_dev);

  gpio_pin_set_dt(&crst, 0);

  flash_power_down(&flash_dev);

  for (;;) {
    k_msleep(333);
  }

  return 1;
}

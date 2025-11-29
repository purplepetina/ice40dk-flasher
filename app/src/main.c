#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zephyr/input/input.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/usb/usbd.h>

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
    HID_USAGE_PAGE(0xFF00), /* Vendor-defined page */
    HID_USAGE(0x01),        /* Vendor usage */
    HID_COLLECTION(HID_COLLECTION_APPLICATION),

    /* All bytes are 0..255 */
    HID_LOGICAL_MIN8(0x00),
    HID_LOGICAL_MAX8(0xFF),

    /* 64 bytes of data */
    HID_REPORT_SIZE(8),   /* 8 bits per field (1 byte) */
    HID_REPORT_COUNT(64), /* 64 of those -> 64 bytes */

    HID_USAGE(0x01), /* "Data buffer" */
    HID_END_COLLECTION,
};

static void mouse_iface_ready(const struct device *dev, const bool ready) {
  LOG_INF("HID device %s interface is %s", dev->name,
          ready ? "ready" : "not ready");
}

static int mouse_get_report(const struct device *dev, const uint8_t type,
                            const uint8_t id, const uint16_t len,
                            uint8_t *const buf) {
  LOG_WRN("Get Report not implemented, Type %u ID %u", type, id);

  return 0;
}

static const struct hid_device_ops hid_ops = {.iface_ready = mouse_iface_ready,
                                              .get_report = mouse_get_report};

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

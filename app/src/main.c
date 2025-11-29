#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>

#include "hid_device.h"
#include "usbd_init.h"
#include "w25q16_hal.h"

LOG_MODULE_REGISTER(main);

/* Device tree node labels */
#define RESET_PIN_NODE DT_NODELABEL(crst)
#define FLASH_NODE DT_NODELABEL(w25q16)

/* SPI operation configuration */
#define SPI_OP_CONFIG (SPI_OP_MODE_MASTER | SPI_WORD_SET(8))

/* Timing constants */
#define RESET_PULSE_MS 2
#define IDLE_SLEEP_MS 1000

#if !DT_NODE_HAS_STATUS(FLASH_NODE, okay)
#error "Flash device w25q16 devicetree node not found or disabled"
#endif

/* Static device configurations */
static struct gpio_dt_spec reset_pin = GPIO_DT_SPEC_GET(RESET_PIN_NODE, gpios);
static struct flash_config flash_dev = {
    .dev = SPI_DT_SPEC_GET(FLASH_NODE, SPI_OP_CONFIG),
};

/**
 * @brief Initialize the FPGA reset pin
 *
 * @return 0 on success, negative errno on failure
 */
static int init_reset_pin(void) {
  int err;

  err = gpio_pin_configure_dt(&reset_pin, GPIO_OUTPUT_ACTIVE);
  if (err) {
    LOG_ERR("Failed to configure reset pin: %d", err);
    return err;
  }

  LOG_DBG("Reset pin configured successfully");
  return 0;
}

/**
 * @brief Perform FPGA reset sequence and initialize flash
 */
static void init_flash_device(void) {
  /* Assert reset */
  gpio_pin_set_dt(&reset_pin, 1);
  k_msleep(RESET_PULSE_MS);

  /* Initialize flash communication */
  flash_reset(&flash_dev);
  flash_read_id(&flash_dev);

  /* Release reset */
  gpio_pin_set_dt(&reset_pin, 0);

  LOG_INF("Flash device initialized");
}

/**
 * @brief Initialize USB device subsystem
 *
 * @return Pointer to USB context on success, NULL on failure
 */
static struct usbd_context *init_usb_device(void) {
  struct usbd_context *usbd_ctx;
  int err;

  usbd_ctx = flasher_usbd_init_device(NULL);
  if (!usbd_ctx) {
    LOG_ERR("Failed to initialize USB device");
    return NULL;
  }

  err = usbd_enable(usbd_ctx);
  if (err) {
    LOG_ERR("Failed to enable USB device: %d", err);
    return NULL;
  }

  LOG_INF("USB device enabled");
  return usbd_ctx;
}

int main(void) {
  int err;

  LOG_INF("ICE40 Flasher starting...");

  /* Initialize HID device */
  err = hid_device_init();
  if (err) {
    LOG_ERR("HID initialization failed: %d", err);
    return err;
  }

  /* Initialize and enable USB */
  if (!init_usb_device()) {
    return -EIO;
  }

  /* Initialize reset pin */
  err = init_reset_pin();
  if (err) {
    return err;
  }

  /* Initialize flash device */
  init_flash_device();

  LOG_INF("System ready - HID interface active");

  /* Main idle loop */
  while (1) {
    k_msleep(IDLE_SLEEP_MS);
  }

  return 0;
}

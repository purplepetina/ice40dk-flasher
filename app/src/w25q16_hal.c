#include "w25q16_hal.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(w25q16_hal);

void flash_reset(struct flash_config *dev) {
  // send 0x66 and 0x99 consecutivly;
  // then wait 30us
  uint8_t tx_cmd[8] = {0xff};
  struct spi_buf tx_buf = {
      .buf = tx_cmd,
      .len = sizeof(tx_cmd),
  };
  struct spi_buf_set tx_set = {
      .buffers = &tx_buf,
      .count = 1,
  };

  int err = spi_write_dt(&dev->dev, &tx_set);
  if (err) {
    LOG_ERR("Failed to reset w25q16");
    return;
  }

  k_msleep(10);

  uint8_t power_on[1] = {0xAB};
  tx_buf.buf = power_on;
  tx_buf.len = 1;

  err = spi_write_dt(&dev->dev, &tx_set);
  if (err) {
    LOG_ERR("Failed to Release from Power Down");
    return;
  }

  k_msleep(1);
}

void flash_power_down(struct flash_config *dev) {
  uint8_t tx_cmd[] = {0xB9};

  struct spi_buf tx_buf = {
      .buf = tx_cmd,
      .len = sizeof(tx_cmd),
  };
  struct spi_buf_set tx_set = {
      .buffers = &tx_buf,
      .count = 1,
  };

  int err = spi_write_dt(&dev->dev, &tx_set);
  if (err) {
    LOG_ERR("Failed to power down w25q16");
    return;
  }
}

void flash_read_id(struct flash_config *dev) {
  uint8_t tx_cmd[4] = {0x9F, 0, 0, 0};
  uint8_t rx_data[4] = {0};

  struct spi_buf tx_buf = {
      .buf = tx_cmd,
      .len = sizeof(tx_cmd),
  };

  struct spi_buf_set tx_set = {
      .buffers = &tx_buf,
      .count = 1,
  };

  struct spi_buf rx_buf = {
      .buf = rx_data,
      .len = sizeof(rx_data),
  };
  struct spi_buf_set rx_set = {
      .buffers = &rx_buf,
      .count = 1,
  };

  int err = spi_transceive_dt(&dev->dev, &tx_set, &rx_set);
  if (err) {
    LOG_ERR("spi_transceive_dt failed: %d", err);
    return;
  }

  LOG_INF("JEDEC ID: %02x %02x %02x", rx_data[1], rx_data[2], rx_data[3]);
  return;
}

void flash_chip_erase(struct flash_config *dev) {
  uint8_t tx_cmd[2] = {0xC7, 0x60};
  struct spi_buf tx_buf = {
      .buf = tx_cmd,
      .len = sizeof(tx_cmd),
  };

  struct spi_buf_set tx_set = {
      .buffers = &tx_buf,
      .count = 1,
  };

  int err = spi_write_dt(&dev->dev, &tx_set);
  if (err) {
    LOG_ERR("chip erase failed %d", err);
    return;
  }
  return;
}

void flash_block_erase_64k(struct flash_config *dev, uint32_t addr_start) {
  uint8_t tx_cmd[4] = {0};
  tx_cmd[0] = 0xd8;
  // put the addr in the next 3 bytes;
  tx_cmd[1] = (uint8_t)((addr_start >> 16) & 0xFF);
  tx_cmd[2] = (uint8_t)((addr_start >> 8) & 0xFF);
  tx_cmd[3] = (uint8_t)(addr_start & 0xFF);

  struct spi_buf tx_buf = {
      .buf = tx_cmd,
      .len = sizeof(tx_cmd),
  };

  struct spi_buf_set tx_set = {
      .buffers = &tx_buf,
      .count = 1,
  };

  int err = spi_write_dt(&dev->dev, &tx_set);
  if (err) {
    LOG_ERR("64kb block erase failed %d", err);
    return;
  }
  return;
}

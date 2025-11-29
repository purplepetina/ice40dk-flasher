/**
 * @file w25q16_hal.c
 * @brief Hardware Abstraction Layer for W25Q16 SPI Flash Memory
 */

#include "w25q16_hal.h"

#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(w25q16_hal);

/* W25Q16 Command Set */
#define W25Q16_CMD_RESET_ENABLE        0x66
#define W25Q16_CMD_RESET_DEVICE        0x99
#define W25Q16_CMD_RELEASE_POWER_DOWN  0xAB
#define W25Q16_CMD_POWER_DOWN          0xB9
#define W25Q16_CMD_READ_JEDEC_ID       0x9F
#define W25Q16_CMD_READ_DATA           0x03
#define W25Q16_CMD_PAGE_PROGRAM        0x02
#define W25Q16_CMD_WRITE_ENABLE        0x06
#define W25Q16_CMD_READ_STATUS_REG1    0x05
#define W25Q16_CMD_CHIP_ERASE          0xC7
#define W25Q16_CMD_BLOCK_ERASE_64K     0xD8

/* Status Register Bits */
#define W25Q16_STATUS_BUSY             0x01

/* Timing delays */
#define RESET_DELAY_MS                 10
#define POWER_DOWN_RELEASE_DELAY_MS    1
#define BUSY_POLL_DELAY_MS             1

/* Buffer sizes */
#define JEDEC_ID_SIZE                  3
#define PAGE_WRITE_SIZE                64

int flash_reset(struct flash_config *dev)
{
	int err;
	uint8_t tx_cmd[8] = {0xFF};

	struct spi_buf tx_buf = {
		.buf = tx_cmd,
		.len = sizeof(tx_cmd),
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};

	/* Send dummy bytes to reset SPI interface */
	err = spi_write_dt(&dev->dev, &tx_set);
	if (err) {
		LOG_ERR("Failed to reset SPI interface: %d", err);
		return err;
	}

	k_msleep(RESET_DELAY_MS);

	/* Release from power-down mode */
	uint8_t power_on_cmd = W25Q16_CMD_RELEASE_POWER_DOWN;
	tx_buf.buf = &power_on_cmd;
	tx_buf.len = 1;

	err = spi_write_dt(&dev->dev, &tx_set);
	if (err) {
		LOG_ERR("Failed to release from power down: %d", err);
		return err;
	}

	k_msleep(POWER_DOWN_RELEASE_DELAY_MS);

	LOG_DBG("Flash reset completed");
	return 0;
}

int flash_power_down(struct flash_config *dev)
{
	int err;
	uint8_t tx_cmd = W25Q16_CMD_POWER_DOWN;

	struct spi_buf tx_buf = {
		.buf = &tx_cmd,
		.len = sizeof(tx_cmd),
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};

	err = spi_write_dt(&dev->dev, &tx_set);
	if (err) {
		LOG_ERR("Failed to enter power down: %d", err);
		return err;
	}

	LOG_DBG("Flash entered power-down mode");
	return 0;
}

int flash_read_id(struct flash_config *dev)
{
	int err;
	uint8_t tx_cmd[4] = {W25Q16_CMD_READ_JEDEC_ID, 0, 0, 0};
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

	err = spi_transceive_dt(&dev->dev, &tx_set, &rx_set);
	if (err) {
		LOG_ERR("Failed to read JEDEC ID: %d", err);
		return err;
	}

	LOG_INF("JEDEC ID: %02X %02X %02X", rx_data[1], rx_data[2], rx_data[3]);
	return 0;
}

int flash_chip_erase(struct flash_config *dev)
{
	int err;
	uint8_t tx_cmd = W25Q16_CMD_CHIP_ERASE;

	struct spi_buf tx_buf = {
		.buf = &tx_cmd,
		.len = sizeof(tx_cmd),
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};

	err = flash_write_enable(dev);
	if (err) {
		return err;
	}

	err = spi_write_dt(&dev->dev, &tx_set);
	if (err) {
		LOG_ERR("Chip erase failed: %d", err);
		return err;
	}

	LOG_DBG("Chip erase initiated");
	return 0;
}

int flash_block_erase_64k(struct flash_config *dev, uint32_t addr_start)
{
	int err;
	uint8_t tx_cmd[4];

	tx_cmd[0] = W25Q16_CMD_BLOCK_ERASE_64K;
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

	err = flash_write_enable(dev);
	if (err) {
		return err;
	}

	err = spi_write_dt(&dev->dev, &tx_set);
	if (err) {
		LOG_ERR("64KB block erase failed: %d", err);
		return err;
	}

	LOG_DBG("64KB block erase at 0x%06X initiated", addr_start);
	return 0;
}

int flash_wait_busy(struct flash_config *dev)
{
	int err;
	uint8_t tx_cmd[2] = {W25Q16_CMD_READ_STATUS_REG1, 0x00};
	uint8_t rx_data[2] = {0};

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

	/* Poll status register until BUSY bit is cleared */
	do {
		err = spi_transceive_dt(&dev->dev, &tx_set, &rx_set);
		if (err) {
			LOG_ERR("Failed to read status register: %d", err);
			return err;
		}
		k_msleep(BUSY_POLL_DELAY_MS);
	} while (rx_data[1] & W25Q16_STATUS_BUSY);

	return 0;
}

int flash_write_enable(struct flash_config *dev)
{
	int err;
	uint8_t tx_cmd = W25Q16_CMD_WRITE_ENABLE;

	struct spi_buf tx_buf = {
		.buf = &tx_cmd,
		.len = sizeof(tx_cmd),
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};

	err = spi_write_dt(&dev->dev, &tx_set);
	if (err) {
		LOG_ERR("Write enable failed: %d", err);
		return err;
	}

	return 0;
}

int flash_write_64bytes(struct flash_config *dev, uint32_t addr,
			const uint8_t *data)
{
	int err;
	uint8_t tx_cmd[4 + PAGE_WRITE_SIZE];

	if (!data) {
		return -EINVAL;
	}

	tx_cmd[0] = W25Q16_CMD_PAGE_PROGRAM;
	tx_cmd[1] = (uint8_t)((addr >> 16) & 0xFF);
	tx_cmd[2] = (uint8_t)((addr >> 8) & 0xFF);
	tx_cmd[3] = (uint8_t)(addr & 0xFF);

	memcpy(&tx_cmd[4], data, PAGE_WRITE_SIZE);

	struct spi_buf tx_buf = {
		.buf = tx_cmd,
		.len = sizeof(tx_cmd),
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};

	err = flash_write_enable(dev);
	if (err) {
		return err;
	}

	err = spi_write_dt(&dev->dev, &tx_set);
	if (err) {
		LOG_ERR("Failed to write 64 bytes at 0x%06X: %d", addr, err);
		return err;
	}

	err = flash_wait_busy(dev);
	if (err) {
		return err;
	}

	LOG_DBG("Wrote 64 bytes at 0x%06X", addr);
	return 0;
}

int flash_read(struct flash_config *dev, uint32_t addr, uint8_t *data,
	       size_t len)
{
	int err;
	uint8_t tx_cmd[4];

	if (!data || len == 0) {
		return -EINVAL;
	}

	tx_cmd[0] = W25Q16_CMD_READ_DATA;
	tx_cmd[1] = (uint8_t)((addr >> 16) & 0xFF);
	tx_cmd[2] = (uint8_t)((addr >> 8) & 0xFF);
	tx_cmd[3] = (uint8_t)(addr & 0xFF);

	struct spi_buf tx_bufs[2] = {
		{
			.buf = tx_cmd,
			.len = sizeof(tx_cmd),
		},
		{
			.buf = NULL,
			.len = len,
		},
	};

	struct spi_buf_set tx_set = {
		.buffers = tx_bufs,
		.count = 2,
	};

	struct spi_buf rx_bufs[2] = {
		{
			.buf = NULL,
			.len = sizeof(tx_cmd),
		},
		{
			.buf = data,
			.len = len,
		},
	};

	struct spi_buf_set rx_set = {
		.buffers = rx_bufs,
		.count = 2,
	};

	err = spi_transceive_dt(&dev->dev, &tx_set, &rx_set);
	if (err) {
		LOG_ERR("Failed to read %zu bytes from 0x%06X: %d",
			len, addr, err);
		return err;
	}

	LOG_DBG("Read %zu bytes from 0x%06X", len, addr);
	return 0;
}


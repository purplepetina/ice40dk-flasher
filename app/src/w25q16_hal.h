/**
 * @file w25q16_hal.h
 * @brief Hardware Abstraction Layer for W25Q16 SPI Flash Memory
 */

#ifndef W25Q16_HAL_H
#define W25Q16_HAL_H

#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <stdint.h>

/**
 * @brief Flash device configuration structure
 */
struct flash_config {
	struct spi_dt_spec dev;
};

/**
 * @brief Reset the flash device
 * 
 * Sends reset sequence and releases from power-down mode
 * 
 * @param dev Pointer to flash device configuration
 * @return 0 on success, negative errno on failure
 */
int flash_reset(struct flash_config *dev);

/**
 * @brief Enter power-down mode
 * 
 * @param dev Pointer to flash device configuration
 * @return 0 on success, negative errno on failure
 */
int flash_power_down(struct flash_config *dev);

/**
 * @brief Read and display JEDEC ID
 * 
 * @param dev Pointer to flash device configuration
 * @return 0 on success, negative errno on failure
 */
int flash_read_id(struct flash_config *dev);

/**
 * @brief Erase entire chip
 * 
 * @param dev Pointer to flash device configuration
 * @return 0 on success, negative errno on failure
 */
int flash_chip_erase(struct flash_config *dev);

/**
 * @brief Erase a 64KB block
 * 
 * @param dev Pointer to flash device configuration
 * @param addr_start Starting address (must be 64KB aligned)
 * @return 0 on success, negative errno on failure
 */
int flash_block_erase_64k(struct flash_config *dev, uint32_t addr_start);

/**
 * @brief Wait for flash busy flag to clear
 * 
 * @param dev Pointer to flash device configuration
 * @return 0 on success, negative errno on failure
 */
int flash_wait_busy(struct flash_config *dev);

/**
 * @brief Enable write operations
 * 
 * @param dev Pointer to flash device configuration
 * @return 0 on success, negative errno on failure
 */
int flash_write_enable(struct flash_config *dev);

/**
 * @brief Write 64 bytes to flash memory
 * 
 * @param dev Pointer to flash device configuration
 * @param addr Starting address
 * @param data Pointer to data buffer (must be at least 64 bytes)
 * @return 0 on success, negative errno on failure
 */
int flash_write_64bytes(struct flash_config *dev, uint32_t addr,
			const uint8_t *data);

/**
 * @brief Read data from flash memory
 * 
 * @param dev Pointer to flash device configuration
 * @param addr Starting address
 * @param data Pointer to destination buffer
 * @param len Number of bytes to read
 * @return 0 on success, negative errno on failure
 */
int flash_read(struct flash_config *dev, uint32_t addr, uint8_t *data,
	       size_t len);

#endif /* W25Q16_HAL_H */


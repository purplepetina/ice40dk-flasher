#ifndef __W25Q16_HAL_H__
#define __W25Q16_HAL_H__

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

struct flash_config {
  struct spi_dt_spec dev;
};

// low level fns
void flash_reset(struct flash_config *dev);
void flash_power_down(struct flash_config *dev);
void flash_read_id(struct flash_config *dev);
void flash_read_dev_id(struct flash_config *dev);

// application level fns

#endif

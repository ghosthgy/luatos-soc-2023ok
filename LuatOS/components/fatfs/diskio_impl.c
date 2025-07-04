/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "luat_base.h"
#include "luat_spi.h"
#include "luat_timer.h"
#include "luat_gpio.h"

#ifdef __LUATOS__
#include "lauxlib.h"
#endif

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

#define LUAT_LOG_TAG "luat.fatfs"
#include "luat_log.h"
uint16_t FATFS_POWER_DELAY = 1;
BYTE FATFS_DEBUG = 0; // debug log, 0 -- disable , 1 -- enable
BYTE FATFS_POWER_PIN = 0xff;

static block_disk_t disks[FF_VOLUMES+1] = {0};

DSTATUS disk_initialize (BYTE pdrv) {
	if (FATFS_DEBUG)
		LLOGD("disk_initialize >> %d", pdrv);
	if (pdrv >= FF_VOLUMES || disks[pdrv].opts == NULL) {
		LLOGD("NOTRDY >> %d", pdrv);
		return RES_NOTRDY;
	}
	if (FATFS_POWER_PIN != 0xff)
	{
		luat_gpio_mode(FATFS_POWER_PIN, Luat_GPIO_OUTPUT, Luat_GPIO_DEFAULT, 0);
		luat_timer_mdelay(FATFS_POWER_DELAY);
		luat_gpio_mode(FATFS_POWER_PIN, Luat_GPIO_OUTPUT, Luat_GPIO_DEFAULT, 1);
	}
	return disks[pdrv].opts->initialize(disks[pdrv].userdata);
}

DSTATUS disk_status (BYTE pdrv) {
	if (FATFS_DEBUG)
		LLOGD("disk_status >> %d", pdrv);
	if (pdrv >= FF_VOLUMES || disks[pdrv].opts == NULL) {
		return RES_NOTRDY;
	}
	return disks[pdrv].opts->status(disks[pdrv].userdata);
}

DRESULT disk_read (BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
	if (FATFS_DEBUG)
		LLOGD("disk_read >> %d", pdrv);
	if (pdrv >= FF_VOLUMES || disks[pdrv].opts == NULL) {
		return RES_NOTRDY;
	}
	return disks[pdrv].opts->read(disks[pdrv].userdata, buff, sector, count);
}

DRESULT disk_write (BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
	//if (FATFS_DEBUG)
	//	LLOGD("disk_write >> %d", pdrv);
	if (pdrv >= FF_VOLUMES || disks[pdrv].opts == NULL) {
		return RES_NOTRDY;
	}
	return disks[pdrv].opts->write(disks[pdrv].userdata, buff, sector, count);
}

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff) {
	if (FATFS_DEBUG)
		LLOGD("disk_ioctl >> %d %d", pdrv, cmd);
	if (pdrv >= FF_VOLUMES || disks[pdrv].opts == NULL) {
		return RES_NOTRDY;
	}
	return disks[pdrv].opts->ioctl(disks[pdrv].userdata, cmd, buff);
}

DRESULT diskio_open(BYTE pdrv, block_disk_t * disk) {
	if (FATFS_DEBUG)
		LLOGD("disk_open >> %d", pdrv);
	if (pdrv >= FF_VOLUMES || disks[pdrv].opts != NULL) {
		return RES_NOTRDY;
	}
	disks[pdrv].opts = disk->opts;
	disks[pdrv].userdata = disk->userdata;
	return RES_OK;
}

#include "luat_malloc.h"
DRESULT diskio_close(BYTE pdrv) {
	if (pdrv >= FF_VOLUMES || disks[pdrv].opts == NULL) {
		return RES_NOTRDY;
	}
	disks[pdrv].opts = NULL;
	if (disks[pdrv].userdata)
		luat_heap_free(disks[pdrv].userdata);
	disks[pdrv].userdata = NULL;
	return RES_OK;
}

DWORD get_fattime (void) {
	
	return ((2020UL-1980) << 25) /* Year = 2010 */
			| (1UL << 21) /* Month = 11 */
			| (1UL << 16) /* Day = 2 */
			| (1U << 11) /* Hour = 15 */
			| (0U << 5) /* Min = 0 */
			| (0U >> 1) /* Sec = 0 */
	;
}


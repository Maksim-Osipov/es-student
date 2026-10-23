#include "device/device.h"
#include "pico/unique_id.h"
#include "pico/version.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"
void device_get_info(device_info_t *info)
{
    if (!info) return;
    volatile const uint32_t *chip=(volatile const uint32_t *)(SYSINFO_BASE+SYSINFO_CHIP_ID_OFFSET);
    uint32_t value=*chip;
    info->board=DEVICE_BOARD;
    pico_get_unique_board_id_string(info->serial, sizeof(info->serial));
    info->manufacturer=(value & SYSINFO_CHIP_ID_MANUFACTURER_BITS)>>SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    info->part=(value & SYSINFO_CHIP_ID_PART_BITS)>>SYSINFO_CHIP_ID_PART_LSB;
    info->revision=(value & SYSINFO_CHIP_ID_REVISION_BITS)>>SYSINFO_CHIP_ID_REVISION_LSB;
    info->sdk_version=PICO_SDK_VERSION_STRING;
}

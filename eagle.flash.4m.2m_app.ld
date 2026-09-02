/* Flash Split for 4M chips - 2MB app + OTA (自定义，用于容纳中文字体) */
/* sketch @0x40200000 (~2036KB) (2084864B) */
/* empty  @0x403FEFF0 (~2048KB) (2097168B) */
/* eeprom @0x405FC000 (4KB) */
/* rfcal  @0x405FE000 (4KB) */
/* wifi   @0x405FF000 (4KB) */

MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40201010, len = 0x1fcff0
}

PROVIDE ( _FS_start = 0x405FC000 );
PROVIDE ( _FS_end = 0x405FC000 );
PROVIDE ( _FS_page = 0x0 );
PROVIDE ( _FS_block = 0x0 );
PROVIDE ( _EEPROM_start = 0x405fc000 );
/* The following symbols are DEPRECATED and will be REMOVED in a future release */
PROVIDE ( _SPIFFS_start = 0x405FC000 );
PROVIDE ( _SPIFFS_end = 0x405FC000 );
PROVIDE ( _SPIFFS_page = 0x0 );
PROVIDE ( _SPIFFS_block = 0x0 );

INCLUDE "local.eagle.app.v6.common.ld"

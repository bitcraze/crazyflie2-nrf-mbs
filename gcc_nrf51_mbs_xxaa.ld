/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x0003F000, LENGTH = 0x0FE0 /* Bootloader on the last 4K of flash. */
  PLATFORMSTRING (r) : ORIGIN = 0x0003FFE0, LENGTH = 0x20 /* 32Byte of platform string */
  RAM (rwx) : ORIGIN = 0x20002000, LENGTH = 0x2000 /* 8 kB is taken by S110,8 kB for app. */
}


INCLUDE "gcc_nrf51_common.ld"

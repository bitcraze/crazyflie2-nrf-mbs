/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie 2.0 nRF51 Master Boot Switch
 * Copyright (c) 2014 Bitcraze AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdint.h>
#include <stdbool.h>
#include <nrf.h>
#include <nrf_gpio.h>

#include <nrf_mbr.h>
#include <nrf_sdm.h>

#include <pinout.h>
#include "crc.h"

#define BOOTLOADER_CONFIG (NRF_UICR_BASE+0x080)

#define FW_ADDRESS 0x00016000

static uint32_t flashPages;
#define FLASH_START  88

#define PAGE_SIZE 1024

#define FLASH_BASE 0x00000000
#define FLASH_SIZE (256*1024)

#define MBR_SIZE (4*1024)
#define MBS_SIZE (4*1024)

#undef FW_FALLBACK

volatile enum {press_short=0, press_long=1, press_verylong=2, press_none=3} press = press_none;

static bool verify_flash_flags();
static void copy_flash();

// Time for timers
#define MS 125UL
#define SEC  1000UL*MS

#ifdef FW_FALLBACK
static sd_mbr_command_t startSdCmd = {
  .command = SD_MBR_COMMAND_INIT_SD,
};
#endif


void blinking_timer(uint32_t period)
{
  NRF_TIMER1->CC[0] = period;
  NRF_TIMER1->EVENTS_COMPARE[0] = 0;
  NRF_TIMER1->TASKS_CLEAR = 1;
  NRF_TIMER1->TASKS_START = 1;
}

void fault(void) __attribute__ ((noreturn));
void fault(void) {
  blinking_timer(50*MS);
  while(1) {
    if (NRF_TIMER1->EVENTS_COMPARE[0]) {
      NRF_TIMER1->EVENTS_COMPARE[0] = 0;
      nrf_gpio_pin_toggle(LED_PIN);
    }
  }
}

void start_firmware() __attribute__ ((noreturn));
void start_firmware()
{
  uint32_t bootloader_address = *(uint32_t*)BOOTLOADER_CONFIG;
  void (*bootloader_start)(void);

  NRF_TIMER0->TASKS_STOP = 1;
  NRF_TIMER1->TASKS_STOP = 1;

  if (bootloader_address != 0xFFFFFFFFUL) {
    bootloader_start = *(void (**)(void))(bootloader_address+4);
    __set_MSP(*(uint32_t*)bootloader_address);
    bootloader_start();
  }
#ifdef FW_FALLBACK
  else if ((press == press_short) &&
             ((*(uint32_t*)(FW_ADDRESS+4)) != 0xFFFFFFFFUL)) {  // As a failsafe start firmware
    void (*fw_start)(void) = *(void (**)(void))(FW_ADDRESS+4);

    NRF_TIMER0->TASKS_STOP = 1;
    NRF_TIMER1->TASKS_STOP = 1;

    sd_mbr_command(&startSdCmd);
    sd_softdevice_forward_to_application(FW_ADDRESS);
    __set_MSP(*(uint32_t*)FW_ADDRESS);
    fw_start();
  }
#endif

  fault();
}

void start_stm_dfu()
{
  // Set current input to 500mA
  nrf_gpio_cfg_output(PM_EN1);
  nrf_gpio_pin_set(PM_EN1);
  nrf_gpio_cfg_output(PM_EN2);
  nrf_gpio_pin_clear(PM_EN2);

  // Starts STM in DFU mode
  nrf_gpio_pin_set(PM_VCCEN_PIN);
  nrf_gpio_cfg_output(STM_BOOT0_PIN);
  nrf_gpio_pin_set(STM_BOOT0_PIN);

  blinking_timer(50*MS);
  while (!NRF_TIMER1->EVENTS_COMPARE[0]);
  while (!NRF_TIMER1->EVENTS_COMPARE[0]);

  blinking_timer(250*MS);
  while(1) {
    if (NRF_TIMER1->EVENTS_COMPARE[0]) {
      NRF_TIMER1->EVENTS_COMPARE[0] = 0;
      nrf_gpio_pin_toggle(LED_PIN);
    }

    if (nrf_gpio_pin_read(BUTTON_PIN) == 0)
      break;
  }

  //Starts STM in FW mode
  nrf_gpio_pin_clear(STM_BOOT0_PIN);
  nrf_gpio_cfg_output(STM_NRST_PIN);
  nrf_gpio_pin_clear(STM_NRST_PIN);

  blinking_timer(50*MS);
  while (!NRF_TIMER1->EVENTS_COMPARE[0]);
  while (!NRF_TIMER1->EVENTS_COMPARE[0]);

  nrf_gpio_cfg_input(STM_NRST_PIN, NRF_GPIO_PIN_PULLUP);

  blinking_timer(150*MS);
  while(1) {
    if (NRF_TIMER1->EVENTS_COMPARE[0]) {
      NRF_TIMER1->EVENTS_COMPARE[0] = 0;
      nrf_gpio_pin_toggle(LED_PIN);
    }
  }
}

int main() __attribute__ ((noreturn));
int main()
{
  press = press_none;

  /* Lock flash for MBR and MBS */
  NRF_MPU->PROTENSET0 = 0x00000001UL;
  NRF_MPU->PROTENSET1 = 0x80000000UL;

  nrf_gpio_cfg_output(LED_PIN);
  nrf_gpio_pin_set(LED_PIN);

  nrf_gpio_cfg_output(PM_VCCEN_PIN);
  nrf_gpio_pin_clear(PM_VCCEN_PIN);

  nrf_gpio_cfg_input(BUTTON_PIN, NRF_GPIO_PIN_PULLUP);

  NRF_CLOCK->TASKS_HFCLKSTART = 1UL;
  while(!NRF_CLOCK->EVENTS_HFCLKSTARTED);

  // Start button timer
  NRF_TIMER0->PRESCALER = 7;   // Prescaler of 16, result frequency is 1MHz
  NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos;
  NRF_TIMER0->CC[0] = 1.5*SEC;   // Launch bootloader long press timing
  NRF_TIMER0->EVENTS_COMPARE[0] = 0;
  NRF_TIMER0->CC[1] = 5*SEC;   // Launch bootloader verylong press timing
  NRF_TIMER0->EVENTS_COMPARE[1] = 0;
  NRF_TIMER0->TASKS_CLEAR = 1;
  NRF_TIMER0->TASKS_START = 1;

  // Setup LED blinking timer
  NRF_TIMER1->PRESCALER = 7;  //Prescaler of 16, result frequency is 1MHz
  NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
  NRF_TIMER1->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;

  // Calculate number of page before the bootloader
  if ((*(uint32_t*)(BOOTLOADER_CONFIG)) > (FLASH_SIZE*PAGE_SIZE))
    fault();
  flashPages = (*(uint32_t*)(BOOTLOADER_CONFIG))/1024;


  if (nrf_gpio_pin_read(BUTTON_PIN) == 0) {
    press = press_short;
  }

  while (nrf_gpio_pin_read(BUTTON_PIN) == 0) {
    if (NRF_TIMER1->EVENTS_COMPARE[0]) {
      NRF_TIMER1->EVENTS_COMPARE[0] = 0;
      nrf_gpio_pin_toggle(LED_PIN);
    }

    if (NRF_TIMER0->EVENTS_COMPARE[0]) {
      NRF_TIMER0->EVENTS_COMPARE[0] = 0;
      blinking_timer(1*SEC);
      press = press_long;
    }

    if (NRF_TIMER0->EVENTS_COMPARE[1]) {
      NRF_TIMER0->EVENTS_COMPARE[1] = 0;
      blinking_timer(250*MS);
      NRF_TIMER0->TASKS_STOP = 1UL;
      press = press_verylong;
    }
  }

  // Saves the detected press in a retained register
  NRF_POWER->GPREGRET &= ~(0x03UL << 1);
  NRF_POWER->GPREGRET |= 0x80 | ((press&0x03UL)<<1);

  if (press != press_verylong) {
    if(verify_flash_flags()) {
      //fault();
      copy_flash();
    } else {
      start_firmware();
    }
  } else {
    start_stm_dfu();
  }

  while(1);
}


/* Copy flash command handling */
typedef struct {
  uint8_t header;        //Should be 0xCF

  uint16_t sdSrcPage;
  uint16_t sdDestPage;
  uint32_t sdLength;
  uint32_t sdCrc32;

  uint16_t blSrcPage;
  uint16_t blDestPage;
  uint32_t blLength;
  uint32_t blCrc32;

  uint32_t crc32;      //CRC32 of the complete structure except crc32
} __attribute__((__packed__)) CopyFlashFlags_t;

static bool verify_flash_flags()
{
  CopyFlashFlags_t *flashFlags = (void*)FLASH_BASE+((flashPages-1)*PAGE_SIZE);
  uint32_t crc = crcSlow(flashFlags, sizeof(CopyFlashFlags_t)-4);
  static sd_mbr_command_t compareCommand = {
      .command = SD_MBR_COMMAND_COMPARE
    };

  // Check that the structure has a good checksum
  if ( (flashFlags->header != 0xcf) || (crc != flashFlags->crc32) ) {
    return false;
  }

  // Check memory boundaries
  if (flashFlags->sdLength != 0) {
    if ( (((flashFlags->sdDestPage*PAGE_SIZE)+flashFlags->sdLength) > (FLASH_SIZE-MBS_SIZE)) ||
         ((flashFlags->sdDestPage*PAGE_SIZE) < MBR_SIZE) ) {
      return false;
    }
  }
  if (flashFlags->blLength != 0) {
    if ( (((flashFlags->blDestPage*PAGE_SIZE)+flashFlags->blLength) > (FLASH_SIZE-MBS_SIZE)) ||
         ((flashFlags->blDestPage*PAGE_SIZE) < MBR_SIZE) ) {
      return false;
    }
  }

  // Check images checksum
  if (flashFlags->sdLength != 0) {
    crc = crcSlow((void*)(FLASH_BASE+(flashFlags->sdSrcPage*PAGE_SIZE)), flashFlags->sdLength);
    if (crc != flashFlags->sdCrc32) {
      return false;
    }
  }
  if (flashFlags->blLength != 0) {
    crc = crcSlow((void*)(FLASH_BASE+(flashFlags->blSrcPage*PAGE_SIZE)), flashFlags->blLength);
    if (crc != flashFlags->blCrc32) {
      return false;
    }
  }

  // Check if everything is already flashed
  if (flashFlags->sdLength != 0) {
    compareCommand.params.compare.len = ((flashFlags->sdLength-1)/4)+1;
    compareCommand.params.compare.ptr1 = (void*)(FLASH_BASE+(flashFlags->sdSrcPage*PAGE_SIZE));
    compareCommand.params.compare.ptr2 = (void*)(FLASH_BASE+(flashFlags->sdDestPage*PAGE_SIZE));
    if (sd_mbr_command(&compareCommand) == NRF_ERROR_NULL)
      return true;
  }

  // Check if everything is already flashed
  if (flashFlags->blLength != 0) {
    compareCommand.params.compare.len = ((flashFlags->blLength+1)/4)+1;
    compareCommand.params.compare.ptr1 = (void*)(FLASH_BASE+(flashFlags->blSrcPage*PAGE_SIZE));
    compareCommand.params.compare.ptr2 = (void*)(FLASH_BASE+(flashFlags->blDestPage*PAGE_SIZE));
    if (sd_mbr_command(&compareCommand) == NRF_ERROR_NULL)
      return true;
  }

  // Erase flash flags
  NRF_NVMC->CONFIG = 2;
  NRF_NVMC->ERASEPAGE = FLASH_BASE+((flashPages-1)*PAGE_SIZE);;
  NRF_NVMC->CONFIG = 0;

  return false;
}

static void copy_flash()
{
  CopyFlashFlags_t *flashFlags = (void*)FLASH_BASE+((flashPages-1)*PAGE_SIZE);
  static sd_mbr_command_t copyCommand = {
    .command = SD_MBR_COMMAND_COPY_SD
  };
  int error;

  if (flashFlags->sdLength != 0) {
    copyCommand.params.copy_sd.len = (((flashFlags->sdLength-1)/1024)+1)*256;
    copyCommand.params.copy_sd.src = (void*)(FLASH_BASE+(flashFlags->sdSrcPage*PAGE_SIZE));
    copyCommand.params.copy_sd.dst = (void*)(FLASH_BASE+(flashFlags->sdDestPage*PAGE_SIZE));
    error = sd_mbr_command(&copyCommand);
    if (error != NRF_SUCCESS)
      fault();
  }

  if (flashFlags->blLength != 0) {
    copyCommand.params.copy_sd.len = (((flashFlags->blLength-1)/1024)+1)*256;
    copyCommand.params.copy_sd.src = (void*)(FLASH_BASE+(flashFlags->blSrcPage*PAGE_SIZE));
    copyCommand.params.copy_sd.dst = (void*)(FLASH_BASE+(flashFlags->blDestPage*PAGE_SIZE));
    error = sd_mbr_command(&copyCommand);
    if (error != NRF_SUCCESS)
      fault();
  }

  // Erase flash flags
  NRF_NVMC->CONFIG = 2;
  NRF_NVMC->ERASEPAGE = FLASH_BASE+((flashPages-1)*PAGE_SIZE);;
  NRF_NVMC->CONFIG = 0;

  //NRF_POWER->GPREGRET |= 0x40;
  press = press_long;
  NRF_POWER->GPREGRET &= ~(0x03UL << 1);
  NRF_POWER->GPREGRET |= 0x80 | ((press&0x03UL)<<1);
  start_firmware();
}



#Put your personal build config in config.mk and DO NOT COMMIT IT!
-include config.mk

CROSS_COMPILE=arm-none-eabi-

CC=$(CROSS_COMPILE)gcc
AS=$(CROSS_COMPILE)as
LD=$(CROSS_COMPILE)gcc
GDB=$(CROSS_COMPILE)gdb

OPENOCD           ?= openocd
OPENOCD_DIR       ?= 
OPENOCD_INTERFACE ?= $(OPENOCD_DIR)interface/stlink-v2.cfg
OPENOCD_TARGET    ?= target/nrf51_stlink.tcl
OPENOCD_CMDS      ?=
O                 ?= -O3

INCLUDES= -I include/nrf -I s110/s110_nrf51822_7.0.0_API/include -I include/cm0 -I include/

PROCESSOR = -mcpu=cortex-m0 -mthumb
NRF= -DNRF51
PROGRAM=nrf_mbs

CFLAGS=$(PROCESSOR) $(NRF) $(INCLUDES) -g3 $(O) -Wall -fdata-sections -ffunction-sections -flto
# --specs=nano.specs -flto
ASFLAGS=$(PROCESSOR)
LDFLAGS=$(PROCESSOR) $(O) -g3 --specs=nano.specs -Wl,-Map=$(PROGRAM).map -Wl,--gc-sections,--entry=Reset_Handler -flto
ifdef SEMIHOSTING
LDFLAGS+= --specs=rdimon.specs -lc -lrdimon
CFLAGS+= -DSEMIHOSTING
endif

ifdef PLATFORM
CFLAGS += -DPLATFORM_$(PLATFORM)
endif

LDFLAGS += -T gcc_nrf51_mbs_xxaa.ld 


OBJS += src/main.o src/crc.o src/platformString.o gcc_startup_nrf51.o system_nrf51.o

all: $(PROGRAM).elf $(PROGRAM).hex $(PROGRAM).bin
	arm-none-eabi-size $(PROGRAM).elf

$(PROGRAM).hex: $(PROGRAM).elf
	arm-none-eabi-objcopy $^ -O ihex $@

$(PROGRAM).bin: $(PROGRAM).elf
	arm-none-eabi-objcopy $^ -O binary $@

$(PROGRAM).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAM).bin $(PROGRAM).elf $(PROGRAM).hex $(PROGRAM).map $(OBJS)


flash: $(PROGRAM).hex
	$(OPENOCD) -d2 -f $(OPENOCD_INTERFACE) $(OPENOCD_CMDS) -f $(OPENOCD_TARGET) -c init -c targets -c "reset halt" \
                 -c "flash write_image erase $(PROGRAM).hex" -c "verify_image $(PROGRAM).hex" -c "reset halt" \
	               -c "mww 0x4001e504 0x01" -c "mww 0x10001014 0x3F000" -c "reset run" -c shutdown

reset:
	$(OPENOCD) -d2 -f $(OPENOCD_INTERFACE) $(OPENOCD_CMDS) -f $(OPENOCD_TARGET) -c init -c targets -c "mww 0x40000544 0x01" -c reset -c shutdown

openocd: $(PROGRAM).elf
	$(OPENOCD) -d2 -f $(OPENOCD_INTERFACE) $(OPENOCD_CMDS) -f $(OPENOCD_TARGET) -c init -c targets

gdb: $(PROGRAM).elf
	$(GDB) -ex "target remote localhost:3333" -ex "monitor reset halt" $^

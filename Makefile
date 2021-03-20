CC=arm-none-eabi-gcc
LD=arm-none-eabi-gcc
AR=arm-none-eabi-ar
OBJCOPY=arm-none-eabi-objcopy
STFLASH=st-flash



# TARGET CONFIG HERE

# Target can be stm32f407, stm32f411.
TARGET=stm32f411
#TARGET=stm32f407

ifeq ($(TARGET), stm32f411)
	# Linker script file, has to be set accordingly to the target
	LDFILE=ld/STM32F411VEHx_FLASH.ld
	# Name of the startup file in the startup/ folder (without the .s extension)           
	STARTUP_FILE=startup_stm32f411xe
	# Value of the preprocessor constant (defining the target) given to $(CC) at compile time          
	TARGET_DEFINE=TARGET_STM32F411
	TARGET_DEFINE_CMSIS=STM32F411xE
else ifeq ($(TARGET), stm32f407)
	LDFILE=ld/STM32F407VGTx_FLASH.ld
	STARTUP_FILE=startup_stm32f407xx
	TARGET_DEFINE=TARGET_STM32F407
	TARGET_DEFINE_CMSIS=STM32F407xx
else
	@echo The TARGET parameter has been wrongly defined in the Makefile. Target does not exist. Build failed.
	exit 1 
endif

# END OF TARGET CONFIG


INCDIR=inc
SRCDIR=src
LIBDIR=lib
OUTDIR=out
DEPDIR=dep
OBJDIR=obj
STARTUPDIR=startup

READERDIR=iso7816-reader
LIBREADER=reader
LIBREADERFILE=$(READERDIR)/lib$(LIBREADER).a

LIB=$(TARGET)_hal
HALDIR=$(LIBDIR)/$(TARGET)/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver
CMSISDIR=$(LIBDIR)/$(TARGET)/STM32CubeF4/Drivers/CMSIS
BSPDIR=$(LIBDIR)/$(TARGET)/STM32CubeF4/Drivers/BSP


MAKEFILE_TESTS=Makefile_tests



OUTPUT_NAME=test
OUTPUT_BIN=$(OUTPUT_NAME).bin
OUTPUT_ELF=$(OUTPUT_NAME).elf
OUTPUT_HEX=$(OUTPUT_NAME).hex
OUTPUT_MAP=$(OUTPUT_NAME).map
OUTPUT_LST=$(OUTPUT_NAME).lst

CPU_CIBLE=cortex-m4


DEFS+= -DUSE_HAL_DRIVER
DEFS+= -DBRIDGE2
DEFS+= -D$(TARGET_DEFINE)
DEFS+= -D$(TARGET_DEFINE_CMSIS)


INCS= -I$(INCDIR)
INCS+= -I$(LIBDIR)/$(TARGET)
INCS+= -I$(READERDIR)/inc
INCS+= -I$(HALDIR)/Inc
INCS+= -I$(HALDIR)/Inc/Legacy
INCS+= -I$(CMSISDIR)/Include
INCS+= -I$(CMSISDIR)/Device/ST/STM32F4xx/Include
INCS+= -I.


CFLAGS= -mcpu=$(CPU_CIBLE)
CFLAGS+= -mlittle-endian
CFLAGS+= -mthumb
CFLAGS+= -Wall
CFLAGS+= -ffunction-sections -fdata-sections
CFLAGS+= -O0
#CFLAGS+= -std=c89
#CFLAGS+= -pedantic-errors
CFLAGS+= $(DEFS)
CFLAGS+= $(INCS)
CFLAGS+= -g


LDFLAGS= -Wl,--gc-sections
LDFLAGS+= -Wl,-Map=$(OUTDIR)/$(OUTPUT_MAP),--cref,--no-warn-mismatch
LDFLAGS+= -L$(LIBDIR)
LDFLAGS+= -l$(LIB)
LDFLAGS+= -L$(READERDIR)
LDFLAGS+= -l$(LIBREADER)
LDFLAGS+= -T$(LDFILE)



SRCS=$(shell ls $(SRCDIR) | sed -e 's/\.s/\.c/g')
SRCS+= $(STARTUP_FILE).c
OBJS=$(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS=$(addprefix $(DEPDIR)/,$(SRCS:.c=.d))




.PHONY: all dirs clean upload library reader tests test report



all:dirs library $(LIBREADERFILE) $(OUTDIR)/$(OUTPUT_ELF) $(OUTDIR)/$(OUTPUT_BIN) $(OUTDIR)/$(OUTPUT_HEX)


upload:all
	$(STFLASH) write $(OUTDIR)/$(OUTPUT_BIN) 0x8000000
	
	
clean:
	rm -rf $(OUTDIR) $(DEPDIR) $(OBJDIR)
	$(MAKE) clean -C $(LIBDIR)
	$(MAKE) clean -C $(READERDIR)
	$(MAKE) --file $(MAKEFILE_TESTS) clean


dirs:
	echo $(OBJS)
	mkdir -p $(OBJDIR) $(DEPDIR) $(OUTDIR)


library:
	$(MAKE) all -C $(LIBDIR)
	

reader:
	$(MAKE) reader -C $(READERDIR)
	
	
tests:
	$(MAKE) --file $(MAKEFILE_TESTS) all
	
test:tests
	$(MAKE) --file $(MAKEFILE_TESTS) test


report:
	$(MAKE) --file $(MAKEFILE_TESTS) report




$(LIBREADERFILE):reader
	

$(OUTDIR)/$(OUTPUT_BIN):$(OUTDIR)/$(OUTPUT_ELF)
	$(OBJCOPY) -O binary $< $@


$(OUTDIR)/$(OUTPUT_HEX):$(OUTDIR)/$(OUTPUT_ELF)
	$(OBJCOPY) -O ihex $< $@


#ajout $(DEPS) en dependances ??
$(OUTDIR)/$(OUTPUT_ELF):$(OBJS)
	$(LD) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	
	

$(OBJDIR)/%.o:$(SRCDIR)/%.c $(DEPDIR)/%.d
	$(CC) $(CFLAGS) -c -MD -MT $*.o -MF $(DEPDIR)/$*.d $< -o $@
	
	
#$(OBJDIR)/%.o:$(SRCDIR)/%.s $(DEPDIR)/%.d
#	$(CC) $(CFLAGS) -c -MD -MT $*.o -MF $(DEPDIR)/$*.d $< -o $@

$(OBJDIR)/$(STARTUP_FILE).o:$(STARTUPDIR)/$(STARTUP_FILE).s $(DEPDIR)/%.d
	$(CC) $(CFLAGS) -c -MD -MT $*.o -MF $(DEPDIR)/$*.d $< -o $@
		
		
$(DEPDIR)/%.d: ;


	
-include $(DEPS)

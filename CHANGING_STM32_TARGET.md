# CHANGING TO ANOTHER STM32 TARGET CHECKLIST #


* Changing linker script to the one corresponding to the target
* Changing preprocessing constants of the ST HAL library
* Changing compilator options (ARM core etc...)
* Check OpenOCD settings for debugging
* Check if GPIO pins are still corresponding to the same functions
* Change startup file
* Uncomment target in /iso7816-reader/lib/stm32f407/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h
* Change board settings in stm32f4xx_hal_conf.h (OSC frequency etc ..)
* Change settings in src/system_stm32f4xx.c
* Change settings in all Makefiles (Makefile, lib/Makefile, iso7816reader/Makefile and iso7816reader/lib/Makefile)
* Change SYSCLK in the reader_hal_comm_settings.h file
* Set AHB, APB, and PLL parameters


################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"C:/ti/ccs2051/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/Ti_workplace/mspm0-modules/Drivers/IMU660RB/Fusion" -I"E:/Ti_workplace/mspm0-modules/Drivers/IMU660RB" -I"E:/Ti_workplace/mspm0-modules/Drivers/LSM6DSV16X" -I"E:/Ti_workplace/mspm0-modules/Drivers/VL53L0X" -I"E:/Ti_workplace/mspm0-modules/Drivers/WIT" -I"E:/Ti_workplace/mspm0-modules/Drivers/BNO08X_UART_RVC" -I"E:/Ti_workplace/mspm0-modules/Drivers/Ultrasonic_GPIO" -I"E:/Ti_workplace/mspm0-modules/Drivers/Ultrasonic_Capture" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Hardware_I2C" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Hardware_SPI" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Software_I2C" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Software_SPI" -I"E:/Ti_workplace/mspm0-modules/Drivers/MPU6050" -I"E:/Ti_workplace/mspm0-modules/mycode" -I"E:/Ti_workplace/mspm0-modules" -I"E:/Ti_workplace/mspm0-modules/Debug" -I"C:/ti/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_10_00_04/source" -I"E:/Ti_workplace/mspm0-modules/Drivers/MSPM0" -DMOTION_DRIVER_TARGET_MSPM0 -DMPU6050 -D__MSPM0G3507__ -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-2003185322: ../mspm0-modules.syscfg
	@echo 'SysConfig - building file: "$<"'
	"C:/ti/sysconfig_1.26.2/sysconfig_cli.bat" -s "C:/ti/mspm0_sdk_2_10_00_04/.metadata/product.json" --script "E:/Ti_workplace/mspm0-modules/mspm0-modules.syscfg" -o "." --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-2003185322 ../mspm0-modules.syscfg
device.opt: build-2003185322
device.cmd.genlibs: build-2003185322
ti_msp_dl_config.c: build-2003185322
ti_msp_dl_config.h: build-2003185322
Event.dot: build-2003185322

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"C:/ti/ccs2051/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/Ti_workplace/mspm0-modules/Drivers/IMU660RB/Fusion" -I"E:/Ti_workplace/mspm0-modules/Drivers/IMU660RB" -I"E:/Ti_workplace/mspm0-modules/Drivers/LSM6DSV16X" -I"E:/Ti_workplace/mspm0-modules/Drivers/VL53L0X" -I"E:/Ti_workplace/mspm0-modules/Drivers/WIT" -I"E:/Ti_workplace/mspm0-modules/Drivers/BNO08X_UART_RVC" -I"E:/Ti_workplace/mspm0-modules/Drivers/Ultrasonic_GPIO" -I"E:/Ti_workplace/mspm0-modules/Drivers/Ultrasonic_Capture" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Hardware_I2C" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Hardware_SPI" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Software_I2C" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Software_SPI" -I"E:/Ti_workplace/mspm0-modules/Drivers/MPU6050" -I"E:/Ti_workplace/mspm0-modules/mycode" -I"E:/Ti_workplace/mspm0-modules" -I"E:/Ti_workplace/mspm0-modules/Debug" -I"C:/ti/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_10_00_04/source" -I"E:/Ti_workplace/mspm0-modules/Drivers/MSPM0" -DMOTION_DRIVER_TARGET_MSPM0 -DMPU6050 -D__MSPM0G3507__ -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: C:/ti/mspm0_sdk_2_10_00_04/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"C:/ti/ccs2051/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/Ti_workplace/mspm0-modules/Drivers/IMU660RB/Fusion" -I"E:/Ti_workplace/mspm0-modules/Drivers/IMU660RB" -I"E:/Ti_workplace/mspm0-modules/Drivers/LSM6DSV16X" -I"E:/Ti_workplace/mspm0-modules/Drivers/VL53L0X" -I"E:/Ti_workplace/mspm0-modules/Drivers/WIT" -I"E:/Ti_workplace/mspm0-modules/Drivers/BNO08X_UART_RVC" -I"E:/Ti_workplace/mspm0-modules/Drivers/Ultrasonic_GPIO" -I"E:/Ti_workplace/mspm0-modules/Drivers/Ultrasonic_Capture" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Hardware_I2C" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Hardware_SPI" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Software_I2C" -I"E:/Ti_workplace/mspm0-modules/Drivers/OLED_Software_SPI" -I"E:/Ti_workplace/mspm0-modules/Drivers/MPU6050" -I"E:/Ti_workplace/mspm0-modules/mycode" -I"E:/Ti_workplace/mspm0-modules" -I"E:/Ti_workplace/mspm0-modules/Debug" -I"C:/ti/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"C:/ti/mspm0_sdk_2_10_00_04/source" -I"E:/Ti_workplace/mspm0-modules/Drivers/MSPM0" -DMOTION_DRIVER_TARGET_MSPM0 -DMPU6050 -D__MSPM0G3507__ -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '



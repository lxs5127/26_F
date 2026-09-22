################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Drivers/MPU6050/MPU6050/%.o: ../Drivers/MPU6050/MPU6050/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/Ti/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"D:/Ti/project/mspm0-modules/Drivers/IMU660RB/Fusion" -I"D:/Ti/project/mspm0-modules/Drivers/IMU660RB" -I"D:/Ti/project/mspm0-modules/Drivers/LSM6DSV16X" -I"D:/Ti/project/mspm0-modules/Drivers/VL53L0X" -I"D:/Ti/project/mspm0-modules/Drivers/WIT" -I"D:/Ti/project/mspm0-modules/Drivers/BNO08X_UART_RVC" -I"D:/Ti/project/mspm0-modules/Drivers/Ultrasonic_GPIO" -I"D:/Ti/project/mspm0-modules/Drivers/Ultrasonic_Capture" -I"D:/Ti/project/mspm0-modules/Drivers/OLED_Hardware_I2C" -I"D:/Ti/project/mspm0-modules/Drivers/OLED_Hardware_SPI" -I"D:/Ti/project/mspm0-modules/Drivers/OLED_Software_I2C" -I"D:/Ti/project/mspm0-modules/Drivers/OLED_Software_SPI" -I"D:/Ti/project/mspm0-modules/Drivers/MPU6050" -I"D:/Ti/project/mspm0-modules/mycode" -I"D:/Ti/project/mspm0-modules" -I"D:/Ti/project/mspm0-modules/Debug" -I"D:/Ti/mspm0_sdk_2_05_01_00/source/third_party/CMSIS/Core/Include" -I"D:/Ti/mspm0_sdk_2_05_01_00/source" -I"D:/Ti/project/mspm0-modules/Drivers/MSPM0" -DMOTION_DRIVER_TARGET_MSPM0 -DMPU6050 -D__MSPM0G3507__ -gdwarf-3 -MMD -MP -MF"Drivers/MPU6050/MPU6050/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '



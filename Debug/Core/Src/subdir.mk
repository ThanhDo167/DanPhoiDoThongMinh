################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/actuator.c \
../Core/Src/dht11.c \
../Core/Src/display1.c \
../Core/Src/keypad1.c \
../Core/Src/lcd_i2c.c \
../Core/Src/logic.c \
../Core/Src/main.c \
../Core/Src/sensor1.c \
../Core/Src/stepper.c \
../Core/Src/stm32f1xx_hal_msp.c \
../Core/Src/stm32f1xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_state.c \
../Core/Src/system_stm32f1xx.c \
../Core/Src/uart_cli.c 

OBJS += \
./Core/Src/actuator.o \
./Core/Src/dht11.o \
./Core/Src/display1.o \
./Core/Src/keypad1.o \
./Core/Src/lcd_i2c.o \
./Core/Src/logic.o \
./Core/Src/main.o \
./Core/Src/sensor1.o \
./Core/Src/stepper.o \
./Core/Src/stm32f1xx_hal_msp.o \
./Core/Src/stm32f1xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_state.o \
./Core/Src/system_stm32f1xx.o \
./Core/Src/uart_cli.o 

C_DEPS += \
./Core/Src/actuator.d \
./Core/Src/dht11.d \
./Core/Src/display1.d \
./Core/Src/keypad1.d \
./Core/Src/lcd_i2c.d \
./Core/Src/logic.d \
./Core/Src/main.d \
./Core/Src/sensor1.d \
./Core/Src/stepper.d \
./Core/Src/stm32f1xx_hal_msp.d \
./Core/Src/stm32f1xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_state.d \
./Core/Src/system_stm32f1xx.d \
./Core/Src/uart_cli.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"


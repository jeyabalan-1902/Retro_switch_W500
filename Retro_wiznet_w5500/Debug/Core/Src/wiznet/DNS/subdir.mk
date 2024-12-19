################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/wiznet/DNS/dns.c 

OBJS += \
./Core/Src/wiznet/DNS/dns.o 

C_DEPS += \
./Core/Src/wiznet/DNS/dns.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/wiznet/DNS/%.o Core/Src/wiznet/DNS/%.su Core/Src/wiznet/DNS/%.cyclo: ../Core/Src/wiznet/DNS/%.c Core/Src/wiznet/DNS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../Core/Inc -I"C:/Users/kjeya/Documents/RTOS/Workspace/Retro_wiznet_w5500/Core/Src/wiznet" -I"C:/Users/kjeya/Documents/RTOS/Workspace/Retro_wiznet_w5500/Core/Src/wiznet/DHCP" -I"C:/Users/kjeya/Documents/RTOS/Workspace/Retro_wiznet_w5500/Core/Src/wiznet/DNS" -I"C:/Users/kjeya/Documents/RTOS/Workspace/Retro_wiznet_w5500/Core/Src/wiznet/MQTT/MQTTPacket" -I"C:/Users/kjeya/Documents/RTOS/Workspace/Retro_wiznet_w5500/Core/Src/wiznet/MQTT" -I"C:/Users/kjeya/Documents/RTOS/Workspace/Retro_wiznet_w5500/Core/Src/wiznet/W5500" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-wiznet-2f-DNS

clean-Core-2f-Src-2f-wiznet-2f-DNS:
	-$(RM) ./Core/Src/wiznet/DNS/dns.cyclo ./Core/Src/wiznet/DNS/dns.d ./Core/Src/wiznet/DNS/dns.o ./Core/Src/wiznet/DNS/dns.su

.PHONY: clean-Core-2f-Src-2f-wiznet-2f-DNS


################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/RConn.cpp \
../src/RUDP.cpp \
../src/buffer.cpp \
../src/config.cpp 

OBJS += \
./src/RConn.o \
./src/RUDP.o \
./src/buffer.o \
./src/config.o 

CPP_DEPS += \
./src/RConn.d \
./src/RUDP.d \
./src/buffer.d \
./src/config.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



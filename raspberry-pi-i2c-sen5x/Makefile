#
# makefile for sen5x_mqtt
#

common_sources = sensirion_config.h sensirion_common.h sensirion_common.c
i2c_sources = sensirion_i2c_hal.h sensirion_i2c.h sensirion_i2c.c
sen5x_sources = sen5x_i2c.h sen5x_i2c.c

common_objects = sensirion_common.o
i2c_objects = sensirion_i2c.o
sen5x_objects = sen5x_i2c.o

i2c_implementation ?= sensirion_i2c_hal.c

CC = gcc
CFLAGS = -Os -Wall -fstrict-aliasing -Wstrict-aliasing=1 -Wsign-conversion -fPIC -I. 


.PHONY: all clean

all: sen5x_mqtt 

sen5x_mqtt: clean

	$(CC) -c -g $(CFLAGS)   ${sen5x_sources} ${i2c_sources} ${i2c_implementation} \
							${common_sources} sen5x_mqtt.c -lmosquitto -lm 

	$(CC)  $(CFLAGS) -g -o  sen5x_mqtt  sensirion_common.o sensirion_i2c.o sen5x_i2c.o sen5x_mqtt.o sensirion_i2c_hal.o -lmosquitto -lm 

		 
mqtt_pub: clean
	$(CC) $(CFLAGS) -o $@  mqtt_pub.c
			
clean:
	$(RM) sen5x_mqtt
	$(RM) mqtt_pub

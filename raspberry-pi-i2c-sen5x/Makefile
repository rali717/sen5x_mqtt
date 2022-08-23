common_sources = sensirion_config.h sensirion_common.h sensirion_common.c
i2c_sources = sensirion_i2c_hal.h sensirion_i2c.h sensirion_i2c.c
sen5x_sources = sen5x_i2c.h sen5x_i2c.c

i2c_implementation ?= sensirion_i2c_hal.c
CC = gcc
CFLAGS = -Os -Wall -fstrict-aliasing -Wstrict-aliasing=1 -Wsign-conversion -fPIC -I. -lm

ifdef CI
    CFLAGS += -Werror
endif

.PHONY: all clean

# all: sen5x_i2c_example_usage
all: sen5x_i2c_mqtt 

sen5x_i2c_mqtt: clean
# sen5x_i2c_example_usage: clean
	$(CC) $(CFLAGS) -o $@  ${sen5x_sources} ${i2c_sources} \
		${i2c_implementation} ${common_sources} sen5x_i2c_mqtt.c -lmosquitto
#		${i2c_implementation} ${common_sources} sen5x_i2c_example_usage.c 


mqtt_pub: clean
	$(CC) $(CFLAGS) -o $@  mqtt_pub.c
			
clean:
#	$(RM) sen5x_i2c_example_usage
	$(RM) sen5x_i2c_mqtt
	$(RM) mqtt_pub

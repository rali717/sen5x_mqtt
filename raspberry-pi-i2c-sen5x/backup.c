/*
 * I2C-Generator: 0.3.0
 * Yaml Version: 2.1.3
 * Template Version: 0.7.0-109-gb259776
 */
/*
 * Copyright (c) 2021, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>   // NAN
#include <stdio.h>  // printf
#include <string.h>

#include "sen5x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

#include <stdio.h>
#include <mosquitto.h>


int main(int argc, char *argv[]) {
    
       if (argc >= 2 ) {
        if ((strcmp(argv[1],"--help") == 0)||
            (strcmp(argv[1],"-h")==0)){
                printf("\n  -h\n --help");
                printf("\n --broker");
                printf("\n --port");
                printf("\n --username");
                printf("\n --password");
                printf("\n --sleep_ms\n\n");
                printf("\n --rest_time_ms\n\n");
                return 0;        
        }
    }
    
    if (argc == 3 ) {
        if ((strcmp(argv[1],"--brokerip") == 0)||
            (strcmp(argv[1],"-b") == 0)){
                printf("\n\nMqtt-Broker-IP:\n %s\n\n\n", argv[2]);                
        }
    }    
        
    if (argc == 1 ) {
        printf("\n\nSet broker-ip to \nlocalhost / 127.0.0.1 \n\n\n");
    }
    
    
    // topic
    // port
    
   
    int16_t error = 0;

    sensirion_i2c_hal_init();

    error = sen5x_device_reset();
    if (error) {
        printf("Error executing sen5x_device_reset(): %i\n", error);
    }

    unsigned char serial_number[32];
    uint8_t serial_number_size = 32;
    error = sen5x_get_serial_number(serial_number, serial_number_size);
    if (error) {
        printf("Error executing sen5x_get_serial_number(): %i\n", error);
    } else {
        printf("Serial number: %s\n", serial_number);
    }

    unsigned char product_name[32];
    uint8_t product_name_size = 32;
    error = sen5x_get_product_name(product_name, product_name_size);
    if (error) {
        printf("Error executing sen5x_get_product_name(): %i\n", error);
    } else {
        printf("Product name: %s\n", product_name);
    }

    uint8_t firmware_major;
    uint8_t firmware_minor;
    bool firmware_debug;
    uint8_t hardware_major;
    uint8_t hardware_minor;
    uint8_t protocol_major;
    uint8_t protocol_minor;
    error = sen5x_get_version(&firmware_major, &firmware_minor, 
                                &firmware_debug,
                                &hardware_major, &hardware_minor,
                                &protocol_major, &protocol_minor);
    if (error) {
        printf("Error executing sen5x_get_version(): %i\n", error);
    } else {
        printf("Firmware: %u.%u, Hardware: %u.%u\n", firmware_major,
               firmware_minor, hardware_major, hardware_minor);
    }

    // set a temperature offset in degrees celsius
    // Note: supported by SEN54 and SEN55 sensors
    // By default, the temperature and humidity outputs from the sensor
    // are compensated for the modules self-heating. If the module is
    // designed into a device, the temperature compensation might need
    // to be adapted to incorporate the change in thermal coupling and
    // self-heating of other device components.
    //
    // A guide to achieve optimal performance, including references
    // to mechanical design-in examples can be found in the app note
    // “SEN5x – Temperature Compensation Instruction” at www.sensirion.com.
    // Please refer to those application notes for further information
    // on the advanced compensation settings used in
    // `sen5x_set_temperature_offset_parameters`,
    // `sen5x_set_warm_start_parameter` and `sen5x_set_rht_acceleration_mode`.
    //
    // Adjust temp_offset to account for additional temperature offsets
    // exceeding the SEN module's self heating.
    float temp_offset = 0.0f;
    error = sen5x_set_temperature_offset_simple(temp_offset);
    if (error) {
        printf("Error executing sen5x_set_temperature_offset_simple(): %i\n",
               error);
    } else {
        printf("Temperature Offset set to %.2f °C (SEN54/SEN55 only)\n",
               temp_offset);
    }

    // Start Measurement
    error = sen5x_start_measurement();

    if (error) {
        printf("Error executing sen5x_start_measurement(): %i\n", error);
    }

   // for (int i = 0; i < 600; i++) {
   
        float mass_concentration_pm1p0;
        float mass_concentration_pm2p5;
        float mass_concentration_pm4p0;
        float mass_concentration_pm10p0;
        float ambient_humidity;
        float ambient_temperature;
        float voc_index;
        float nox_index;
        
                    // {"name":"sen54","temp_c":"13.3","humidity":"59.7"}
            char jsonstring[1024]="";
            char temp_string[10]="";
            char string1[10]="";
            char string2[10]="";
            char string3[10]="";
            char dot_string[]=".";
            char tagname[255]="Unknown Location\0";
        //  char hum_string[10]="";
             
                     
            char jsonstring_Start[]="{\"product_name\":\"";             
            char jsonstring_serial_number[]="\",\"serial_number\":\"";            
            char jsonstring_hardware[]="\",\"hardware\":[";
            char jsonstring_firmware[]="],\"firmware\":[";            
            char jsonstring_protocol[]="],\"protocol\":[";  
            char jsonstring_tagname[]="],\"tagname\":\"";            
            char jsonstring_temp_c[]="\",\"temp_c\":";            
            char jsonstring_hum[]=",\"hum\":";            
            char jsonstring_pm1p0[]=",\"mass_concentration_pm1p0\":";            
            char jsonstring_p2p5[]=",\"mass_concentration_pm2p5\":";            
            char jsonstring_pm4p0[]=",\"mass_concentration_pm4p0\":";            
            char jsonstring_pm10p0[]=",\"mass_concentration_pm10p0\":";            
            char jsonstring_voc[]=",\"voc_index\":";            
            char jsonstring_nox[]=",\"nox_index\":";                            
            char jsonstring_End[]="}\0";
            
            
        while(1) {
        // Read Measurement
   //     sensirion_i2c_hal_sleep_usec(15000000); // 1s  1000ms
        sensirion_i2c_hal_sleep_usec(10000000); //10s



        error = sen5x_read_measured_values(
            &mass_concentration_pm1p0, &mass_concentration_pm2p5,
            &mass_concentration_pm4p0, &mass_concentration_pm10p0,
            &ambient_humidity, &ambient_temperature, &voc_index, &nox_index);
        if (error) {
            printf("Error executing sen5x_read_measured_values(): %i\n", error);
        } else {
            printf("Mass concentration pm1p0: %.1f µg/m³\n",
                   mass_concentration_pm1p0);
            printf("Mass concentration pm2p5: %.1f µg/m³\n",
                   mass_concentration_pm2p5);
            printf("Mass concentration pm4p0: %.1f µg/m³\n",
                   mass_concentration_pm4p0);
            printf("Mass concentration pm10p0: %.1f µg/m³\n",
                   mass_concentration_pm10p0);
            if (isnan(ambient_humidity)) {
                printf("Ambient humidity: n/a\n");
            } else {
                printf("Ambient humidity: %.1f %%RH\n", ambient_humidity);
            }
            if (isnan(ambient_temperature)) {
                printf("Ambient temperature: n/a\n");
            } else {
                printf("Ambient temperature: %.1f °C\n", ambient_temperature);
            }
            if (isnan(voc_index)) {
                printf("Voc index: n/a\n");
            } else {
                printf("Voc index: %.1f\n", voc_index);
            }
            if (isnan(nox_index)) {
                printf("Nox index: n/a\n");
            } else {
                printf("Nox index: %.1f\n", nox_index);
            }
            
            // MQTT
            int rc;
            struct mosquitto * mosq;

            mosquitto_lib_init();


            mosq = mosquitto_new("publisher-test", true, NULL);

            //mosquitto_username_pw_set(mosq,"admin","password");
            
            rc = mosquitto_connect(mosq, "localhost", 1883, 60);
            if(rc != 0){
                printf("Client could not connect to broker! Error Code: %d\n", rc);
                mosquitto_destroy(mosq);
                return -1;
            }
            printf("We are now connected to the broker!\nTry to send message.");


            //  temp_offset
            
            // Building mqtt-json-string
            strcpy(jsonstring, jsonstring_Start);  //Json-Start + product_name
            strcat(jsonstring, product_name);
            
            strcat(jsonstring, jsonstring_serial_number);
            strcat(jsonstring, serial_number);
            
            strcat(jsonstring, jsonstring_hardware);
            sprintf(string1,"%u",hardware_major);
            sprintf(string2,"%u",hardware_minor);
            strcat(string1, ","); 
            strcat(string1, string2); 
            strcat(jsonstring, string1); 
            
                                     
            strcat(jsonstring, jsonstring_firmware);
            sprintf(string1,"%u",firmware_major);
            sprintf(string2,"%u",firmware_minor);
            strcat(string1, ","); 
            strcat(string1, string2); 
            strcat(jsonstring, string1);  
            
            strcat(jsonstring, jsonstring_protocol);
            sprintf(string1,"%u",protocol_major);
            sprintf(string2,"%u",protocol_minor);
            strcat(string1, ","); 
            strcat(string1, string2); 
            strcat(jsonstring, string1); 
   
            strcat(jsonstring, jsonstring_tagname);
            strcat(jsonstring, tagname);
               
            strcat(jsonstring, jsonstring_temp_c);
            sprintf(string1,"%.2f",ambient_temperature); // temp
            strcat(jsonstring, string1);    
                                        
            strcat(jsonstring, jsonstring_hum);  //"hum":"
            sprintf(string1,"%.2f",ambient_humidity);
            strcat(jsonstring, string1);
         
            strcat(jsonstring, jsonstring_pm1p0);  //pm1p0
            sprintf(string1,"%.1f",mass_concentration_pm1p0);
            strcat(jsonstring, string1);
         
            strcat(jsonstring, jsonstring_p2p5);  //pm2p5
            sprintf(string1,"%.1f",mass_concentration_pm2p5);
            strcat(jsonstring, string1);
         
            strcat(jsonstring, jsonstring_pm4p0);  //pm4p0
            sprintf(string1,"%.1f",mass_concentration_pm4p0);
            strcat(jsonstring, string1);
            
            strcat(jsonstring, jsonstring_pm10p0);  //pm10p0
            sprintf(string1,"%.1f",mass_concentration_pm10p0);
            strcat(jsonstring, string1);
            
            strcat(jsonstring, jsonstring_voc);  //voc_index
            sprintf(string1,"%.1f",voc_index);
            strcat(jsonstring, string1);
            
  //          strcat(jsonstring, jsonstring_nox);  //nox_index
  //          sprintf(temp_string,"%.1f",nox_index);
  //          strcat(jsonstring, temp_string);
         
            strcat(jsonstring,jsonstring_End);

            printf(jsonstring);

            mosquitto_publish(mosq, NULL, "sensors/sen54", strlen(jsonstring), jsonstring, 0, false);
            //mosquitto_publish(mosq, NULL, "sensor/sen54-1", 6, "Hambient_humidity_ambient_temperature_", 0, false);
                                                        
            

            mosquitto_disconnect(mosq);
            mosquitto_destroy(mosq);

            mosquitto_lib_cleanup();
            // return 0;
            // End MQTT
            }
    }

    error = sen5x_stop_measurement();
    if (error) {
        printf("Error executing sen5x_stop_measurement(): %i\n", error);
    }

    return 0;
}

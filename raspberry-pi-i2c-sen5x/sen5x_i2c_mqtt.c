
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "sen5x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

#include <mosquitto.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

//-----------------------------------------------------------------------------

struct Delayed_Start {
    bool done;
    int delay_s;
    time_t seconds_t0;
    time_t seconds_now;
} delayed_start;

bool delayed_start_done = false;
int delayed_start_duration_s =
    100;  // 100 seconds sleep, before sending mqtt-packages
time_t start_time_t0_s;

// --- sensirion sen5x struct ---
typedef struct {
    unsigned char product_name[32];
    uint8_t product_name_size;
    unsigned char serial_number[32];
    uint8_t serial_number_size;
    uint8_t firmware_major;
    uint8_t firmware_minor;
    uint8_t hardware_major;
    uint8_t hardware_minor;
    uint8_t protocol_major;
    uint8_t protocol_minor;
    float temp_offset;
    bool firmware_debug;
    float mass_concentration_pm1p0;
    float mass_concentration_pm2p5;
    float mass_concentration_pm4p0;
    float mass_concentration_pm10p0;
    float ambient_humidity;
    float ambient_temperature;
    float voc_index;
    float nox_index;
    float hum_abs;
    unsigned char tagname[256];
} Sen5x;

//--- Strings for building mqtt-json-strings ---
const char jsonstring_Start[] = "{\"product_name\":\"";
const char jsonstring_serial_number[] = "\",\"serial_number\":\"";
const char jsonstring_hardware[] = "\",\"hardware\":[";
const char jsonstring_firmware[] = "],\"firmware\":[";
const char jsonstring_protocol[] = "],\"protocol\":[";
const char jsonstring_tagname[] = "],\"tagname\":\"";
const char jsonstring_temp_c[] = "\",\"temp_c\":";
const char jsonstring_hum[] = ",\"hum\":";
const char jsonstring_pm1p0[] = ",\"mass_concentration_pm1p0\":";
const char jsonstring_p2p5[] = ",\"mass_concentration_pm2p5\":";
const char jsonstring_pm4p0[] = ",\"mass_concentration_pm4p0\":";
const char jsonstring_pm10p0[] = ",\"mass_concentration_pm10p0\":";
const char jsonstring_voc[] = ",\"voc_index\":";
const char jsonstring_nox[] = ",\"nox_index\":";
const char jsonstring_hum_abs[] = ",\"hum_abs_g_m3\":";
const char jsonstring_End[] = "}\0";

int16_t get_sensor_data(Sen5x* sen5x) {

    sensirion_i2c_hal_sleep_usec(10000000);  // 10s
    int16_t error = sen5x_read_measured_values(
        &(sen5x->mass_concentration_pm1p0), &(sen5x->mass_concentration_pm2p5),
        &(sen5x->mass_concentration_pm4p0), &(sen5x->mass_concentration_pm10p0),
        &(sen5x->ambient_humidity), &(sen5x->ambient_temperature),
        &(sen5x->voc_index), &(sen5x->nox_index));

    if (error) {
        printf("Error executing sen5x_read_measured_values(): %i\n", error);
    } else {
        printf("Mass concentration pm1p0: %.1f µg/m³\n",
               sen5x->mass_concentration_pm1p0);
        printf("Mass concentration pm2p5: %.1f µg/m³\n",
               sen5x->mass_concentration_pm2p5);
        printf("Mass concentration pm4p0: %.1f µg/m³\n",
               sen5x->mass_concentration_pm4p0);
        printf("Mass concentration pm10p0: %.1f µg/m³\n",
               sen5x->mass_concentration_pm10p0);

        if (isnan(sen5x->ambient_humidity)) {
            printf("Ambient humidity: n/a\n");
        } else {
            printf("Ambient humidity: %.1f %%RH\n", sen5x->ambient_humidity);
        }
        if (isnan(sen5x->ambient_temperature)) {
            printf("Ambient temperature: n/a\n");
        } else {
            printf("Ambient temperature: %.1f °C\n",
                   sen5x->ambient_temperature);
        }
        if (isnan(sen5x->voc_index)) {
            printf("Voc index: n/a\n");
        } else {
            printf("Voc index: %.1f\n", sen5x->voc_index);
        }
        if (isnan(sen5x->nox_index)) {
            printf("Nox index: n/a\n");
        } else {
            printf("Nox index: %.1f\n", sen5x->nox_index);
        }
        if (isnan(sen5x->hum_abs)) {
            printf("Humidity absolute  g/m3: n/a\n");
        } else {
            printf("Humidity absolute  g/m3: %.1f\n", sen5x->hum_abs);
        }
    }
    return error;
}

//=========================================================================

// MQTT
int16_t send_mqtt_msg(Sen5x* sen5x) {

    if (!delayed_start_done) {
        if (start_time_t0_s + delayed_start_duration_s > time(NULL) ||
            time(NULL) - start_time_t0_s >
                delayed_start_duration_s +
                    1) {  // if time was set to new time (timediff too high),
                          // than stop waiting

            delayed_start_done = true;
        }
    }
    if (!delayed_start_done) {
        return;
    }

    // if (!delayed_start.done) {
    //     delayed_start.seconds_now = time(NULL);
    //     if (delayed_start.seconds_now >=
    //         (delayed_start.seconds_t0 +
    //             delayed_start.delay_s))  // <- normal delay-time-check
    //         // ||
    //         //     delayed_start.seconds_now - delayed_start.seconds_t0 >
    //         //         delayed_start.delay_s + 1  // if time was changed,
    //         //     || delayed_start.seconds_t0 -
    //         delayed_start.seconds_now >
    //         //            delayed_start.delay_s + 1  // stop delaying
    //         mqtt
    //         {
    //             delayed_start.done = true;
    //             printf("\nSen5x: delayed mqtt-start done  ( %i
    //             seconds).",
    //                    delayed_start.delay_s);
    //         }
    //     if (!delayed_start.done) {
    //         return true;
    //     }
    // }

    char jsonstring[2048] = "";  // MQTT-Message-JSON-String
    char temp_string[30] = "";
    char string1[30] = "";
    char string2[30] = "";

    int rc;
    struct mosquitto* mosq;

    mosquitto_lib_init();

    mosq = mosquitto_new("publisher-test", true, NULL);

    // mosquitto_username_pw_set(mosq,"admin","password");

    rc = mosquitto_connect(mosq, "localhost", 1883, 60);

    if (rc != 0) {
        printf("Client could not connect to broker! Error Code: %d\n", rc);
        mosquitto_destroy(mosq);
        return -1;
    }

    printf("\nBroker connected!\nTry to send message.\n");

    //  temp_offset

    // --- Building mqtt-json-string ---
    strcpy(jsonstring, jsonstring_Start);  // Json-Start + product_name ...
    strcat(jsonstring, (char*)sen5x->product_name);

    strcat(jsonstring, jsonstring_serial_number);
    strcat(jsonstring, (char*)sen5x->serial_number);

    strcat(jsonstring, jsonstring_hardware);
    sprintf(string1, "%u", sen5x->hardware_major);
    sprintf(string2, "%u", sen5x->hardware_minor);
    strcat(string1, ",");
    strcat(string1, string2);
    strcat(jsonstring, string1);

    strcat(jsonstring, jsonstring_firmware);
    sprintf(string1, "%u", sen5x->firmware_major);
    sprintf(string2, "%u", sen5x->firmware_minor);
    strcat(string1, ",");
    strcat(string1, string2);
    strcat(jsonstring, string1);

    strcat(jsonstring, jsonstring_protocol);
    sprintf(string1, "%u", sen5x->protocol_major);
    sprintf(string2, "%u", sen5x->protocol_minor);
    strcat(string1, ",");
    strcat(string1, string2);
    strcat(jsonstring, string1);

    strcat(jsonstring, jsonstring_tagname);
    strcat(jsonstring, (char*)sen5x->tagname);

    strcat(jsonstring, jsonstring_temp_c);
    sprintf(string1, "%.2f", sen5x->ambient_temperature);  // temp
    strcat(jsonstring, string1);

    strcat(jsonstring, jsonstring_hum);  //"hum":"
    sprintf(string1, "%.2f", sen5x->ambient_humidity);
    strcat(jsonstring, string1);

    strcat(jsonstring, jsonstring_pm1p0);  // pm1p0
    sprintf(string1, "%.1f", sen5x->mass_concentration_pm1p0);
    strcat(jsonstring, string1);

    strcat(jsonstring, jsonstring_p2p5);  // pm2p5
    sprintf(string1, "%.1f", sen5x->mass_concentration_pm2p5);
    strcat(jsonstring, string1);

    strcat(jsonstring, jsonstring_pm4p0);  // pm4p0
    sprintf(string1, "%.1f", sen5x->mass_concentration_pm4p0);
    strcat(jsonstring, string1);

    strcat(jsonstring, jsonstring_pm10p0);  // pm10p0
    sprintf(string1, "%.1f", sen5x->mass_concentration_pm10p0);
    strcat(jsonstring, string1);

    strcat(jsonstring, jsonstring_voc);  // voc_index
    sprintf(string1, "%.1f", sen5x->voc_index);
    strcat(jsonstring, string1);

    if (!isnan(sen5x->nox_index)) {
        strcat(jsonstring, jsonstring_nox);  // nox_index
        sprintf(temp_string, "%.1f", sen5x->nox_index);
        strcat(jsonstring, temp_string);
    }

    if (!isnan(sen5x->hum_abs)) {
        strcat(jsonstring, jsonstring_hum_abs);  // humidity absolut in g/m3
        sprintf(temp_string, "%.1f", sen5x->hum_abs);
        strcat(jsonstring, temp_string);
    }

    strcat(jsonstring, jsonstring_End);

    printf(jsonstring);
    printf("\n");

    mosquitto_publish(mosq, NULL, "sensors/sen54", strlen((size_t)jsonstring),
                      jsonstring, 0, false);

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);

    mosquitto_lib_cleanup();

    return 0;
    // End MQTT
}

//=============================================================================

int setup_sen5x(Sen5x* sen5x) {

    sensirion_i2c_hal_init();

    int16_t error = 0;
    int16_t error_counter = 0;

    error = sen5x_device_reset();
    if (error) {
        printf("Error executing sen5x_device_reset(): %i\n", error);
        error_counter++;
    }

    error = sen5x_get_serial_number(sen5x->serial_number,
                                    sen5x->serial_number_size);
    if (error) {
        printf("Error executing sen5x_get_serial_number(): %i\n", error);
        error_counter++;
    } else {
        printf("Serial number: %s\n", sen5x->serial_number);
    }

    error =
        sen5x_get_product_name(sen5x->product_name, sen5x->product_name_size);
    if (error) {
        printf("Error executing sen5x_get_product_name(): %i\n", error);
        error_counter++;
    } else {
        printf("Product name: %s\n", sen5x->product_name);
    }

    error =
        sen5x_get_version(&(sen5x->firmware_major), &(sen5x->firmware_minor),
                          &(sen5x->firmware_debug), &(sen5x->hardware_major),
                          &(sen5x->hardware_minor), &(sen5x->protocol_major),
                          &(sen5x->protocol_minor));

    if (error) {
        printf("Error executing sen5x_get_version(): %i\n", error);
        error_counter++;
    } else {
        printf("Firmware: %u.%u, Hardware: %u.%u\n", sen5x->firmware_major,
               sen5x->firmware_minor, sen5x->hardware_major,
               sen5x->hardware_minor);
    }

    error = sen5x_set_temperature_offset_simple(sen5x->temp_offset);
    if (error) {
        printf("Error executing sen5x_set_temperature_offset_simple(): %i\n",
               error);
        error_counter++;
    } else {
        printf("Temperature Offset set to %.2f °C (SEN54/SEN55 only)\n",
               sen5x->temp_offset);
    }

    // Start Measurement
    error = sen5x_start_measurement();
    if (error) {
        printf("Error executing sen5x_start_measurement(): %i\n", error);
        error_counter++;
    }

    if (error > 0) {
        printf("\n\nInit sen5x / Errorcounter: %i\n", error_counter);
        return -1;
    } else {
        return 0;
    }
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

void parse_args(int argc, char* argv[]) {
    for (int i = 0; i <= argc; i++) {
        printf(argv[i]);
    }
}
//-----------------------------------------------------------------------------

float get_absolute_hum_g_m3(float temp, float rel_hum) {
    float abs_hum = NAN;

    // https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/

    abs_hum = (float)((6.112 * exp(((17.67 * temp) / (243.5 + temp))) *
                       rel_hum * 2.1674) /
                      (273.15 + temp));

    return abs_hum;
}

//-----------------------------------------------------------------------------

//===---  main
//---=============================================================

int main(int argc, char* argv[]) {

    //   parse_args(argc, &argc[])

    if (argc >= 2) {
        if ((strcmp(argv[1], "--help") == 0) || (strcmp(argv[1], "-h") == 0)) {
            printf("\n  -h\n --help");
            printf("\n --broker");
            printf("\n --port");
            printf("\n --username");
            printf("\n --password");
            printf("\n --sleep_ms\n\n");
            printf("\n --rest_time_msn\n");
            printf("\n --temp_offset\n");
            return 0;
        }
    }

    if (argc == 3) {
        if ((strcmp(argv[1], "--brokerip") == 0) ||
            (strcmp(argv[1], "-b") == 0)) {
            printf("\n\nMqtt-Broker-IP:\n %s\n\n\n", argv[2]);
        }
    }

    if (argc == 1) {
        printf("\n\nSet broker-ip to \nlocalhost / 127.0.0.1 \n\n\n");
    }

    // topic
    // port
    // delayed_start_seconds 240

    // struct Delayed_Start delayed_start;
    delayed_start.done = false;
    delayed_start.delay_s =
        180;  // In the first 180 seconds - no mqtt message send
    delayed_start.seconds_t0 = time(NULL);

    delayed_start_done = false;
    delayed_start_duration_s = 100;
    start_time_t0_s = time(NULL);

    //-----------------------------------------------------------------------------

    Sen5x sen54;
    sen54.product_name_size = 32;
    sen54.serial_number_size = 32;
    sen54.temp_offset = 0.0f;

    strcpy((char*)sen54.tagname, (char*)"No comment\0");

    int16_t error = 0;

    // --- Main-Loop ------------------------------------------------
    // --- Init sensor + read out parameter + get meassurements + send mqtt

    while (1) {

        do {
            error = setup_sen5x(&sen54);
        } while (error);

        do {
            error = get_sensor_data(
                &sen54);  // get new sensor-data // Read Measurement
            if (error) {
                printf("\n\nFailt getting sensordata\n");
                break;
            }
            sen54.hum_abs = get_absolute_hum_g_m3(sen54.ambient_temperature,
                                                  sen54.ambient_humidity);
            error = send_mqtt_msg(&sen54);
            // send via mqtt
        } while (!error);

        error = sen5x_stop_measurement();
        if (error) {
            printf("Error executing sen5x_stop_measurement(): %i\n", error);
        }

        sleep(5);
        printf("\n\nRestart sen5x.\n\n");
    }
    return 1;
}

//==== End of main
//====================================================================

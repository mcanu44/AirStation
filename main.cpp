/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "platform/mbed_thread.h"
#include "BMP280.h"
#include "Adafruit_SSD1306.h"

 
// Blinking rate in milliseconds
#define BLINKING_RATE_MS 5000
#define SLEEP_PMS PTE20
#define RESET_PMS PTE21
#define TOP_LED PTA5

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut pms_sleep(SLEEP_PMS);
DigitalOut pms_reset(RESET_PMS);
DigitalOut top_led(TOP_LED);

BMP280 sensor(PTC11, PTC10); //SDA,SCL, addr
BufferedSerial pms(PTE22, PTE23, 9600);
BufferedSerial gps(PTE0, PTE1, 9600);
BufferedSerial pc(USBTX, USBRX, 115200);

float temp, pres, hum;
char s_pres[10];
char s_temp[10];

struct PM_value{
    uint16_t pm1;
    uint16_t pm2_5;
    uint16_t pm10;
};

class I2CPreInit : public I2C
{
public:
    I2CPreInit(PinName sda, PinName scl) : I2C(sda, scl)
    {
        frequency(400000);
        start();
    };
};

void serial_printf(BufferedSerial &serial, const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    serial.write(buffer, strlen(buffer));
}

FileHandle* mbed::mbed_override_console(int fd)
{
    return &pc;
}

I2CPreInit gI2C(PTC9,PTC8);
Adafruit_SSD1306_I2c oled(gI2C, PTA5);

const int BUFFER_SIZE = 32;
uint8_t buffer[BUFFER_SIZE];

// Fonction pour lire les données du capteur
PM_value readPMSData() {
    // Commande pour demander des données en mode passif
    PM_value result;

    uint8_t cmd[] = {0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71};
    pms.write(cmd, sizeof(cmd));
    result.pm1 = 0;
    result.pm2_5 = 0;
    result.pm10 = 0;

    ThisThread::sleep_for(1s);  // Attente de la réponse du capteur
    pms.
    if (pms.readable()) {
        int num = pms.read(buffer, BUFFER_SIZE);
        
        if (num == 32 && buffer[0] == 0x42 && buffer[1] == 0x4D) {
            result.pm1 = (buffer[10] << 8) | buffer[11];
            result.pm2_5 = (buffer[12] << 8) | buffer[13];
            result.pm10 = (buffer[14] << 8) | buffer[15];
            
            printf("PM1.0: %u %cg/m%c, PM2.5: %u μg/m³, PM10: %u μg/m³\n", result.pm1, char(181), char(179), result.pm2_5, result.pm10);
            return result;
        } else {
            return result;
        }
    } else {
        return result;
    }
}

void init_pms() {
    // Initialisation du capteur
    pms_sleep = true;
    pms_reset = true;
    pms_reset = false; //reset
    thread_sleep_for(5);
    pms_reset = true;
    thread_sleep_for(5);
    uint8_t comd[] = {0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70};
    pms.write(comd, sizeof(comd));
}

int main()
{
    // Initialise the digital pin LED1 as an output
    PM_value particules;
    sensor.initialize();
    init_pms();

    oled.printf("%ux%u OLED Display\r\n", oled.width(), oled.height());
    oled.display();
    thread_sleep_for(500);
    pc.write("Temperature et pression : \n\n", 28);
    thread_sleep_for(1000);
    oled.clearDisplay();
    
    while (true) {
        led1 = !led2;
        led2 = !led3;
        led3 = !led1;
        top_led = 1;
        thread_sleep_for(100); //ms
        top_led = 0;
        temp = sensor.getTemperature();
        pres = sensor.getPressure();
        //hum = sensor.getHumidity();
        particules = readPMSData();
        oled.setTextCursor(1,1);
        oled.printf("%2.2f %cC, %4.2f hPa\n\r", temp, char(247), pres);
        oled.setTextCursor(1,9);
        oled.printf("PM1 : %i mg/m3", particules.pm1);
        oled.setTextCursor(1,17);
        oled.printf("PM2.5 : %i mg/m3", particules.pm2_5);
        oled.setTextCursor(1,25);
        oled.printf("PM10 : %i mg/m3", particules.pm10);
        oled.display();
        
        printf("%2.2f*C - %4.2f hPa\n\r", temp, pres); //renvoi sur la console
        //thread_sleep_for(BLINKING_RATE_MS);
        ThisThread::sleep_for(10s);
    }
}

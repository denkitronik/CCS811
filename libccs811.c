/*
 * The MIT License
 *
 * Copyright 2018 Alvaro Salazar <alvaro@denkitronik.com>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* 
 * File:   main.c
 * Author: Alvaro Salazar <alvaro@denkitronik.com>
 * Created on 13 de octubre de 2018, 04:20 PM
 */
#include <wiringPi.h> 
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libccs811.h>

char addressPin=CCS811_ADDR_HIGH;   ///< Address pin in the Raspberry PI (use wiringpi pinout)
char resetPin = 3;                  ///< Reset pin in the Raspberry PI (use wiringpi pinout)
char interruptPin = 2;              ///< Interrupt pin in the Raspberry PI (use wiringpi pinout)
int file;                           ///< File descriptor, a nonnegative integer can be used in subsequent system calls (read(), write(), lseek(), fcntl(), etc.)

char * getSelectedRegister(char registerSelected);          ///<
int selectRegister(const char reg);                         ///<
int readI2C(const int numBytes);                            ///<
int writeI2CBytes(const char reg, int numBytes);            ///<
const char getRegister(const char reg, const int numBytes); ///<

/**
 * Get the current application register of the CCS811. \n It is no valid until readRegister() be called.
 * @return The current application register
 */
union ApplicationRegister getAppRegister() {
    return appReg;
}

/**
 * 
 * @param appRegister
 */
void setAppRegister(union ApplicationRegister appRegister){
    appReg=appRegister;
}

/**
 * Resets the CCS811 using the specified pin in setResetPin() function
 */
void resetCCS811() {
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, LOW);
    digitalWrite(resetPin, HIGH);
}

/**
 * Set the Reset pin in the Raspberry PI (use wiringpi pinout)
 * @param pin number wiringpi style where the reset pin is connected
 */
void setResetPin(char pin) {
    resetPin=pin;
}

/**
 * Set the Interrupt pin in the Raspberry PI (use wiringpi pinout)
 * @param pin WiringPi pin number where the interrupt pin is connected
 */
void setInterruptPin(char pin) {
    interruptPin=pin;
}

/**
 * Set the Address pin in the Raspberry PI (use wiringpi pinout). \n It has no effect until connectCCS811() is called.
 * @param pin WiringPi pin number where the address pin is connected. \n If 0: The pin is not connected
 */
void setAddressPin(char pin) {
    addressPin=pin;
}

/**
 * Specify enviroment data to adjust the measurement of eCO2 and eTVOC levels
 * @param temperature Environment's temperature in degree celsius
 * @param humidity Environment's humidity in % Relative Humidity
 * @return Error code. Valid values are: ERROR, OK
 */
int setEnvironmentData(float temperature, float humidity) {
    char humidityInteger = (char) humidity;
    char temperatureInteger = (char) temperature;
    clearAppReg();
    appReg.env_data.humidity = humidityInteger;
    appReg.env_data.humidity_fraction = (uint16_t) ((humidity - humidityInteger)*512);
    appReg.env_data.temperature = temperatureInteger + 25;
    appReg.env_data.temperature_fraction = (uint16_t) ((temperature - temperatureInteger)*512);
    if(writeRegister(ENV_DATA)==ERROR) return ERROR;
    return OK;
}

/**
 * Prints the current Application Register
 */
void printAppReg() {
    printf("printAppReg:\t\t");
    for (char i = 0; i < 8; i++) printf("0x%x ", appReg.buffer[i]);
    printf("\r\n");
}

/**
 * Prints the status register
 */
int printStatus() {
    union ApplicationRegister appRegAux;
    appRegAux = appReg;
    if(readRegister(STATUS)==ERROR) return ERROR;
    printf("El app_valid es 0x%x\r\n", appReg.status.app_valid);
    printf("El data_ready es 0x%x\r\n", appReg.status.data_ready);
    printf("El error es 0x%x\r\n", appReg.status.error);
    printf("El fwmode es 0x%x\r\n", appReg.status.fwmode);
    appReg = appRegAux;
    return OK;
}

/**
 * Check if there is a data available
 * @return State of the interrupt pin settled with setInterruptPin() function
 */
uint8_t available() {
    pinMode(interruptPin, INPUT);
    return digitalRead(interruptPin);
}

/**
 * Clear the current Application Registers
 */
void clearAppReg() {
    for (char i = 0; i < 8; i++) appReg.buffer[i] = 0;
}

/**
 * Prints to stdout the state of errors
 * @param message Title of the printing in the stdout
 * @return Error code. Valid values are: ERROR, OK
 */
void printErrors(char * message) {
    union ApplicationRegister appRegAux;
    appRegAux = appReg;
    char error = 0;
    readRegister(ERROR_ID);
    printf("............................\r\n", message);
    printf(".......%s ERRORS........\r\n", message);
    printf("............................\r\n", message);
    if (appReg.error_id.heater_fault) {
        printf("The Heater current in the CCS811 is not in range.\r\n");
        error = 1;
    }
    if (appReg.error_id.heater_supply) {
        printf("The Heater voltage is not being applied correctly.\r\n");
        error = 1;
    }
    if (appReg.error_id.max_resistance) {
        printf("The sensor resistance measurement has reached or exceeded the maximum range.\r\n");
        error = 1;
    }
    if (appReg.error_id.meas_mode_invalid) {
        printf("Request to write an unsupported mode to MEAS_MODE.\r\n");
        error = 1;
    }
    if (appReg.error_id.read_reg_invalid) {
        printf("Read request to a mailbox ID that is invalid.\r\n");
        error = 1;
    }
    if (appReg.error_id.write_reg_invalid) {
        printf("Write request to a mailbox ID that is invalid.\r\n");
        error = 1;
    }
    if (!error) {
        printf("No errors happened!\r\n");
    }
    printf("............................\r\n", message);
    appReg = appRegAux;
}

/**
 * Read a number of bytes of the register specified. Use it in this way (example): readRegister(STATUS) \n Valid values are (these constants contains reg and numBytes): STATUS MEAS_MODE ALG_RESULT_DATA RAW_DATA ENV_DATA NTC THRESHOLDS BASELINE HW_ID HW_VERSION FW_BOOT_VERSION FW_APP_VERSION INTERNAL_STATE ERROR_ID APP_ERASE APP_DATA APP_VERIFY APP_START SW_RESET
 * @param reg Register to be read. 
 * @param numBytes Number of bytes to be read from the register.
 * @return Error code. Valid values are: ERROR, OK 
 */
int readRegister(const char reg, int numBytes) {
    if(selectRegister(reg)==OK){ 
        if(readI2C(numBytes)==OK){
            if (getRegister(ALG_RESULT_DATA) == getRegister(reg, 0))
                if (appReg.alg_result_data.eco2 > 255) {
                    appReg.alg_result_data.eco2 = ((appReg.alg_result_data.eco2 >> 8) | (appReg.alg_result_data.eco2 << 8));
                    if (appReg.alg_result_data.eco2 > 32768) appReg.alg_result_data.eco2 = appReg.alg_result_data.eco2 - 32768;
                }
            if (appReg.alg_result_data.tvoc > 255) {
                appReg.alg_result_data.tvoc = ((appReg.alg_result_data.tvoc >> 8) | (appReg.alg_result_data.tvoc << 8));
                if (appReg.alg_result_data.tvoc > 32768) appReg.alg_result_data.tvoc = appReg.alg_result_data.tvoc - 32768;
            }
            return OK;
        } else return ERROR;
    } else return ERROR;
}

/**
 * Returns the register specified in the "CCS811 constants" because they contain reg and numBytes concatenated: STATUS MEAS_MODE ALG_RESULT_DATA RAW_DATA ENV_DATA NTC THRESHOLDS BASELINE HW_ID HW_VERSION FW_BOOT_VERSION FW_APP_VERSION INTERNAL_STATE ERROR_ID APP_ERASE APP_DATA APP_VERIFY APP_START SW_RESET
 * @param reg The register specified
 * @param numBytes The number of bytes of the register specified in reg to be read
 * @return The register selected
 */
const char getRegister(const char reg, const int numBytes) {
    return reg;
}

/**
 * Prints information about the impact of the eCO2 in the human health.\n Messages taken from: https://www.kane.co.uk/knowledge-centre/what-are-safe-levels-of-co-and-co2-in-rooms
 * @param eco2 Level of the eCO2 in ppm
 */
void printHealthECO2(uint16_t eco2) {
    printf("Health eCO2:");
    if ((eco2 >= 250)&&(eco2 <= 350)) printf("Normal background concentration in outdoor ambient air.");
    if ((eco2 > 350)&&(eco2 <= 1000)) printf("Concentrations typical of occupied indoor spaces with good air exchange.");
    if ((eco2 > 1000)&&(eco2 <= 2000)) printf("Complaints of drowsiness and poor air.");
    if ((eco2 > 2000)&&(eco2 < 5000)) printf("Headaches, sleepiness and stagnant, stale, stuffy air. Poor concentration, loss of attention, increased heart rate and slight nausea may also be present.");
    if ((eco2 >= 5000)&&(eco2 < 40000)) printf("Workplace exposure limit (as 8-hour TWA) in most jurisdictions.");
    if (eco2 >= 40000) printf("Exposure may lead to serious oxygen deprivation resulting in permanent brain damage, coma, even death.");
    printf("\r\n");
}

/**
 * Prints information about the impact of the eTVOC in the human health. Taken of TVOC guidelines issued by the German Federal Environmental Agency
 * @param tvoc Level of the eTVOC in ppm
 */
void printHealthVOC(uint16_t tvoc) {
    printf("Health eTVOC:");
    if ((tvoc >= 0)&&(tvoc <= 65)) printf("[Excellent][No objections][Target value][No limit exposure time]");
    if ((tvoc > 65)&&(tvoc <= 220)) printf("[Good][No relevant objections][Ventilation / airing recommended][No limit exposure time]");
    if ((tvoc > 220)&&(tvoc <= 660)) printf("[Moderate][Some objections][Intensified ventilation / airing recommended. Search for sources][Exposure time < 12 months]");
    if ((tvoc > 660)&&(tvoc < 2200)) printf("[Poor][Major objections][ntensified ventilation / airing necessary. Search for sources][Exposure time < 1 month]");
    if ((tvoc >= 2200)&&(tvoc < 5500)) printf("[Unhealty][Situation not acceptable][Use only if unavoidable / Intense ventilation necessary ][Exposure time only hours]");
    if (tvoc > 5500) printf("[Unhealty][Situation not acceptable][Can not be used / Intense ventilation necessary][Exposure time: no time]");
    printf("\r\n");
}

/**
 * Write appReg in the specified register 
 * @param reg Register to be written
 * @param numBytes Number of bytes to be written
 * @return Error code. Valid values are: ERROR, OK
 */
int writeRegister(const char reg, const int numBytes) {
    return writeI2CBytes(reg, numBytes);
}

/**
 * Connect to the CCS811 plugged in the I2C bus with the specified address 
 * @param address Address of the CCS811.\n Only two valid address: CCS811_ADDR_HIGH and CCS811_ADDR_LOW
 * @return Error code. Valid values are: ERROR, OK
 */
int connectCCS811(const char address) {
    if(addressPin!=0) {
        switch(address){
            case CCS811_ADDR_HIGH:  digitalWrite(addressPin,HIGH); break;
            case CCS811_ADDR_LOW:   digitalWrite(addressPin,LOW);  break;
            default:                digitalWrite(addressPin,HIGH); break;;
        }
    }
    char *filename = "/dev/i2c-1";
    if ((file = open(filename, O_RDWR)) < 0) {
        DEBUG("Connecting to the I2C bus failed");
        return ERROR;
    }
    if (ioctl(file, I2C_SLAVE, address) < 0) {
        DEBUG("Setting the CCS811 as slave failed\r\n");
        return ERROR;
    }
    readRegister(HW_ID);
    if (appReg.hw_id == 0x81) {
        writeRegister(STATUS);
        writeRegister(APP_START);
        return OK;
    } else {
        DEBUG("HW_ID = 0x81 does not match with this device.\r\n");
        return ERROR;
    }
}

/**
 * Read a number of bytes of the selected register
 * @param numBytes Number of bytes to be read
 * @return Error code. Valid values are: ERROR, OK
 */
int readI2C(const int numBytes) {
    clearAppReg();
    if (read(file, appReg.buffer, numBytes) != numBytes) {
        DEBUG("Reading the I2C bus failed\r\n");
        return ERROR;
    } else {
        DEBUG("readI2C:\t\t");
        for (char i = 0; i < 8; i++) DEBUG("0x%x ", appReg.buffer[i]);
        DEBUG("count=%d\r\n", numBytes);
        return OK;
    }
}

/**
 * Selects a register to be read
 * @param reg Register to be read
 * @return Error code. Valid values are: ERROR, OK
 */
int selectRegister(const char reg) {
    const char buffer[1] = {reg};
    if (write(file, buffer, 1) != 1) {
        DEBUG("Writing to the CCS811 failed (I2C transaction failed).\r\n");
        return ERROR;
    } else {
        DEBUG(getSelectedRegister(buffer[0]));
        return OK;
    }
}

/**
 * Get a string with the name of the selected register
 * @param registerSelected Selected register
 * @return String with the name of the selected register
 */
char * getSelectedRegister(char registerSelected) {
    switch (registerSelected) {
        case 0x00: return "STATUS\r\n";
            break;
        case 0x01: return "MEAS_MODE\r\n";
            break;
        case 0x02: return "ALG_RESULT_DATA\r\n";
            break;
        case 0x03: return "RAW_DATA\r\n";
            break;
        case 0x05: return "ENV_DATA\r\n";
            break;
        case 0x06: return "NTC\r\n";
            break;
        case 0x10: return "THRESHOLDS\r\n";
            break;
        case 0x11: return "BASELINE\r\n";
            break;
        case 0x20: return "HW_ID\r\n";
            break;
        case 0x21: return "HW_VERSION\r\n";
            break;
        case 0x23: return "FW_BOOT_VERSION\r\n";
            break;
        case 0x24: return "FW_APP_VERSION\r\n";
            break;
        case 0xA0: return "INTERNAL_STATE\r\n";
            break;
        case 0xE0: return "ERROR_ID\r\n";
            break;
        case 0xF1: return "APP_ERASE\r\n";
            break;
        case 0xF2: return "APP_DATA\r\n";
            break;
        case 0xF3: return "APP_VERIFY\r\n";
            break;
        case 0xF4: return "APP_START\r\n";
            break;
        case 0xFF: return "SW_RESET\r\n";
            break;
    }
}

/**
 * Write a number of bytes of the the prepared appReg.buffer in the specified register
 * @param reg Register to be written
 * @param numBytes Number of bytes of appReg.buffer to be written
 * @return Error code. Valid values are: ERROR, OK
 */
int writeI2CBytes(const char reg, int numBytes) {
    unsigned char bufferData[9] = {reg, 0, 0, 0, 0, 0, 0, 0, 0};
    for (char i = 0; i < numBytes; i++) bufferData[i + 1] = appReg.buffer[i];
    if (write(file, bufferData, (numBytes + 1)) != (numBytes + 1)) {
        /* ERROR HANDLING: i2c transaction failed */
        DEBUG("Writing to the I2C bus failed\r\n");
        return ERROR;
    } else {
        DEBUG("writeI2CBytes: \t\t");
        for (char i = 0; i < 9; i++) DEBUG("0x%x ", bufferData[i]);
        DEBUG("count=%d\r\n", numBytes + 1);
        return OK;
    }
}
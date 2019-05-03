/**
 *  Example using shared library for CCS811 gas Sensor
 *  @version 1.0.0
 *  @author Alvaro Salazar <alvaro@denkitronik.com>
 *  http://www.denkitronik.com
 *
 */

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
 * File:   example.c
 * Author: Alvaro Salazar <alvaro@denkitronik.com>
 *
 * Created on 13 de octubre de 2018, 04:20 PM
 */


#include <stdio.h>
#include <stdlib.h>
#include <libccs811.h>
#include <wiringPi.h>

//#define PRINTERRORS  			//Print errors () method
//#define DEBUG                         //Enable lines to DEBUG

extern union ApplicationRegister appReg; ///< Application register

/**
 * Program to test the libccs811 shared library
 */
int main(int argc, char** argv) {
    float temperature = 28.5F;  //We fix this test value of temperature in degree celsius (Use a precision temperature sensor)
    float humidity = 48.5F;     //We fix this test value of humidity in % relative humidity (Use a precision humidity sensor)
    setbuf(stdout, NULL);       //We wont give any buffering to stout (printf() function) in order to print the message immediately.

    //< Step 1: (Required) Setup the wiringPi library
    wiringPiSetup();            //We need to use the excelent library wiringPi in this version of libccs811.
    
    //< Step 2: (Optional) If you need to reset the CCS811
    //resetCCS811();            //If we have problems in the chip, we can reset it
    //delay(2);                 //After reset we must wait around 2 milliseconds

    //< Step 3: (Required) Set the address pin of the CCS811 in wiringpi style. A value of 0 it means it's  not connected
    setAddressPin(0);
    
    //< Step 4: (Required) Now we connect to the CCS811 in the address specified. Two valid values: CCS811_ADDR_HIGH and CCS811_ADDR_LOW
    connectCCS811(CCS811_ADDR_HIGH);            //Lets choose the address of our CCS811
    
    //< Step 5: (Required) Let's set up the CCS811 (in this example: drive mode each 1 second, enable interrupts, disable threshold interrupt)
    appReg.meas_mode.reserved2 = 0;             //We dont know what reserved bits do, so let's put them zero
    appReg.meas_mode.reserved1 = 0;             //We dont know what reserved bits do, so let's put them zero
    appReg.meas_mode.driveMode = MODE1_EACH_1S; //Lets select the operation mode: MODE0_IDLE MODE1_EACH_1S MODE2_EACH_10S MODE3_EACH_60S MODE4_EACH_250MS
    appReg.meas_mode.int_data_ready = 1;        //Lets enable the interrupt pin after we have a valid data
    appReg.meas_mode.int_thresh = 0;            //We are going to disable the interrupts for thresholds
    
    //printAppReg();                            //Uncomment if you need to print the current application register
    
    //< Step 6: (Required) We write the application register prepared to the MEAS_MODE register (To set the measurement mode)
    writeRegister(MEAS_MODE);                   //Lets write appReg in the MEAS_MODE register
    
    PRINT_ERRORS("DEBUG2: main()");             //If you need to debug, then define PRINT_ERRORS
    
    for (int i = 0; i < 100; i++) {             //We are going to get only 100 samples
        //< Step 7: (Required) We set the current environment data (temperature and humidity)
        setEnvironmentData(temperature, humidity);  //We are setting the current environment temperature and humidity to adjust the measurement
        
        //< Step 8: (Required) Let's wait here until the device triggers an interrupt
        while (available());                    //We have to wait here for a interrupt (it indicates a new valid data is available)
        
        DEBUG("\r\nAvailable data! # %d\r\n", i);   //If you need to debug, then define DEBUG
        
        //< Step 9: (Required) Read the ALG_RESULT_DATA register to get the eCO2 and eTVOC levels
        readRegister(ALG_RESULT_DATA);              //Let's read the ALG_RESULT_REGISTER, (It has the eCO2 and eTVOC values in ppm!)
        
        //< Step 10: (Required) Read the eCO2 and eTVOC levels
        printf("eCO2 %d ppm   ", appReg.alg_result_data.eco2);  //Let's print the eCO2 value
        printf("TVOC %d ppb\r\n", appReg.alg_result_data.tvoc); //Let's print the eTVOC value
        
        //< Step 11: (Optional) You can print some message about human health in these levels of CO2 and VOCs
        printHealthECO2(appReg.alg_result_data.eco2);           //Let's print the impact in the human health of the eCO2 level
        printHealthVOC(appReg.alg_result_data.tvoc);            //Let's print the impact in the human health of the eTVOC level
        
        PRINT_ERRORS("DEBUG4: main()");                         //If you need to debug, then define PRINT_ERRORS
    }
    return (EXIT_SUCCESS);  //Goodbye program
}

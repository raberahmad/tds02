/*
 * Copyright (C) 2018, Hogeschool Rotterdam, Harry Broeders
 * All rights reserved.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <NoRTOS.h>

#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/i2s.h>
#include <ti/drivers/I2C.h>

#include "Board.h"
#include "config.h"

// You can select the sample rate here:
#define SAMPLINGFREQUENCY 48000
#if SAMPLINGFREQUENCY < 8000 || SAMPLINGFREQUENCY > 48000 || SAMPLINGFREQUENCY % 4000 != 0
#error Sampling Frequency must be between 8 kHz and 48 kHz (included) and must be a multiple of 4 kHz.
#endif





void I2S_ISR(void)
{
    static int N = 24000;
    static int16_t buffer[24000];
    static int counter = 0;
    static unsigned long dataLeft;
    static unsigned long output;
    // The 16-bit samples are stored in 32-bit variables because the API also supports 24-bit samples.
    I2SDataGet(I2S_BASE, I2S_DATA_LINE_1, &dataLeft);

    //output = dataLeft + (buffer[counter]>>2);
    output = dataLeft + 3*(buffer[counter]>>3);

    // You can process the 16-bit left sample here.
    I2SDataPut(I2S_BASE, I2S_DATA_LINE_0, output);

    buffer[counter] = output;

    counter = (counter + 1)%N;

    I2SIntClear(I2S_BASE, I2S_INT_XDATA);
}

int main(void)
{
    // Init CC3220S LAUNCHXL board.
    Board_initGeneral();
    // Prepare to use TI drivers without operating system.
    NoRTOS_start();

    printf("line-in_2_line_out: STEREO LINE IN ==> HP LINE OUT.\n");
    printf("Sampling frequency = %d Hz.\n", SAMPLINGFREQUENCY);

    // Configure an I2C connection which is used to configure the audio codec.
    I2C_Handle i2cHandle = ConfigureI2C(Board_I2C0, I2C_400kHz);
    // Configure the audio codec.
    ConfigureAudioCodec(i2cHandle, SAMPLINGFREQUENCY);

    // Configure an I2S connection which is use to send/receive samples to/from the codec.
    ConfigureI2S(PRCM_I2S, I2S_BASE, SAMPLINGFREQUENCY);

    // Register I2S_ISR
    I2SIntRegister(I2S_BASE, I2S_ISR);
    // Enable interrupt to I2S_ISR when I2S is ready to send a sample to the codec
    I2SIntEnable(I2S_BASE, I2S_INT_XDATA);

    while(1);

    return 0;
}


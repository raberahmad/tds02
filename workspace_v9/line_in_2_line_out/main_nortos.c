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
#define SAMPLINGFREQUENCY 44000
#if SAMPLINGFREQUENCY < 8000 || SAMPLINGFREQUENCY > 48000 || SAMPLINGFREQUENCY % 4000 != 0
#error Sampling Frequency must be between 8 kHz and 48 kHz (included) and must be a multiple of 4 kHz.
#endif

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

    while (1)
    {
        unsigned long dataLeft, dataRight;
        // The 16-bit samples are stored in 32-bit variables because the API also supports 24-bit samples.
        I2SDataGet(I2S_BASE, I2S_DATA_LINE_1, &dataLeft);
        // You can process the 16-bit left sample here.
        I2SDataPut(I2S_BASE, I2S_DATA_LINE_0, dataLeft);
        I2SDataGet(I2S_BASE, I2S_DATA_LINE_1, &dataRight);
        // You can process the 16-bit right sample here.
        I2SDataPut(I2S_BASE, I2S_DATA_LINE_0, dataRight);
    }
    return 0;
}

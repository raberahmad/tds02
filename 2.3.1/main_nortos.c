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

// You can select the sample rate here
#define SAMPLINGFREQUENCY 8000
#if SAMPLINGFREQUENCY < 8000 || SAMPLINGFREQUENCY > 48000 || SAMPLINGFREQUENCY % 4000 != 0
#error Sampling Frequency must be between 8 kHz and 48 kHz (included) and must be a multiple of 4 kHz.
#endif

int main(void)
{
    // Init CC3220S LAUNCHXL board.
    Board_initGeneral();
    // Prepare to use TI drivers without operating system
    NoRTOS_start();

    printf("1 kHz sine wave ==> Left HP LINE OUT.\n");
    printf("1 kHz square wave ==> Right HP LINE OUT.\n");

    // Configure an I2C connection which is used to configure the audio codec.
    I2C_Handle i2cHandle = ConfigureI2C(Board_I2C0, I2C_400kHz);
    // Configure the audio codec.
    ConfigureAudioCodec(i2cHandle, SAMPLINGFREQUENCY);

    // Configure an I2S connection which is use to send/receive samples to/from the codec.
    ConfigureI2S(PRCM_I2S, I2S_BASE, SAMPLINGFREQUENCY);

    /* Pre-generated sine wave data, 16-bit signed fixed point samples Q0.15 */
    int16_t sinetable[48] = {
        0x0000, 0x10b4, 0x2120, 0x30fb, 0x3fff, 0x4dea,
        0x5a81, 0x658b, 0x6ed8, 0x763f, 0x7ba1, 0x7ee5,
        0x7ffd, 0x7ee5, 0x7ba1, 0x76ef, 0x6ed8, 0x658b,
        0x5a81, 0x4dea, 0x3fff, 0x30fb, 0x2120, 0x10b4,
        0x0000, 0xef4c, 0xdee0, 0xcf06, 0xc002, 0xb216,
        0xa57f, 0x9a75, 0x9128, 0x89c1, 0x845f, 0x811b,
        0x8002, 0x811b, 0x845f, 0x89c1, 0x9128, 0x9a76,
        0xa57f, 0xb216, 0xc002, 0xcf06, 0xdee0, 0xef4c
    };

    int16_t squaretable[48] = {
            0x8002, 0x8002, 0x8002, 0x8002, 0x8002, 0x8002,
            0x8002, 0x8002, 0x8002, 0x8002, 0x8002, 0x8002,
            0x8002, 0x8002, 0x8002, 0x8002, 0x8002, 0x8002,
            0x8002, 0x8002, 0x8002, 0x8002, 0x8002, 0x8002,
            0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd,
            0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd,
            0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd,
            0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd,
    };
    int16_t sec, msec, sampleNum;
    int16_t dataLeft, dataRight;
    size_t n = 0, m=0;

    for (sec = 0; sec < 5 * 60; sec++) {
        for (msec = 0; msec < 1000; msec++) {
            for (sampleNum = 0; sampleNum < SAMPLINGFREQUENCY/1000; sampleNum++) {
                dataLeft = sinetable[n];
                dataRight = squaretable[m];
                I2SDataPut(I2S_BASE, I2S_DATA_LINE_0, (unsigned long)dataLeft);
                //I2SDataPut(I2S_BASE, I2S_DATA_LINE_0, 0);
                I2SDataPut(I2S_BASE, I2S_DATA_LINE_0, (unsigned long)dataRight);
                n++;
                m++;
                if (n == 48) {
                    n = 0;
                }
                if (m == 48) {
                    m = 0;
                }
            }
        }
    }

    printf("\n**Progam ended**\n");

    return 0;
}

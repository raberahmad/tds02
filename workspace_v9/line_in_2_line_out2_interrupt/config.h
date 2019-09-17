/*
 * Configure functions to use the TI TLV320AIC3254 codec on the
 * CC3200AUDBOOST board with the CC3220S-LAUNCHXL board
 * without using DMA.
 *
 * Copyright (C) 2018, Hogeschool Rotterdam, Harry Broeders
 * All rights reserved.
 *
 * Based on Driver for TI TLV320AIC3110 codec
 * Copyright (C) 2015-2017 Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 */
#ifndef __HR_CONFIG_H__
#define __HR_CONFIG_H__

#include <ti/drivers/I2C.h>

// Configure an I2C connection using the TI I2C driver.
extern I2C_Handle ConfigureI2C(uint_least8_t index, I2C_BitRate bitRate);

// Configure an I2S connection using the TI driverlib API.
extern void ConfigureI2S(unsigned long peripheral, unsigned long base, unsigned int samplingFrequency);

// Functions to configure the codec using an I2C connection
// volume: 0 -> 0 bB (Highest) to 116 -> -72.3 dB (Lowest)
extern void AudioVolumeControl(I2C_Handle i2cHandle, signed char volume);
extern void CodecMute(I2C_Handle i2cHandle);
extern void CodecUnmute(I2C_Handle i2cHandle);
// Codec configure:
    // PGA (Programmable Gain Amplifier) = 0 dB.
    // Headphone Output = enabled.
    // Line outputs (to class D amplifier) = disabled.
    // ADC gain = 0 dB.
    // AGC (Automatic Gain Control) = disabled.
    // ADC processing block = PRB_R1 (default).
    // Microphone = disabled.
    // DAC processing block = PRB_P1 (default).
    // DRC (Dynamic Gain Compression) = disabled.
extern void ConfigureAudioCodec(I2C_Handle i2cHandle, unsigned int samplingFrequency);

#endif

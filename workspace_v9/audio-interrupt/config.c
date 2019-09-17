/*
 * Configure functions to use the TI TLV320AIC3254 codec on the
 * CC3200AUDBOOST board with the CC3220S-LAUNCHXL board
 * without using DMA.
 *
 * Copyright (C) 2018, Hogeschool Rotterdam, Harry Broeders
 * All rights reserved.
 *
 * Based on Driver for TI TLV320AIC3110 CODEC
 * Copyright (C) 2015-2017 Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <ti/devices/cc32xx/inc/hw_common_reg.h>
#include <ti/devices/cc32xx/inc/hw_i2c.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/rom.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/i2c.h>
#include <ti/devices/cc32xx/driverlib/i2s.h>
#include <ti/devices/cc32xx/driverlib/pin.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/utils.h>

#include <ti/drivers/I2C.h>
#include <ti/drivers/power/PowerCC32XX.h>

#include "config.h"

// Configure an I2C connection using the TI I2C driver.

I2C_Handle ConfigureI2C(uint_least8_t index, I2C_BitRate bitRate)
{
    I2C_Handle i2cHandle;
    I2C_Params i2cParams;
    I2C_init();
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = bitRate;
    i2cHandle = I2C_open(index, &i2cParams);
    if (i2cHandle == NULL) {
        // Error initializing I2C.
        while (1);
    }
    return i2cHandle;
}

// Configure an I2S connection which is use to send/receive samples to/from the codec.

void ConfigureI2S(unsigned long peripheral, unsigned long base, unsigned int samplingFrequency)
{
    // Register power dependency. Keeps the I2S clock running in SLP and DSLP modes.
    int_fast16_t ret = Power_setDependency(PowerCC32XX_PERIPH_I2S);
    if (ret != Power_SOK) {
        // Error setting power dependency.
        while (1);
    }

    // There is no TI I2S driver (without DMA) available so the TI driverlib API is used.
    PRCMPeripheralReset(peripheral);
    I2SEnable(base, I2S_MODE_TX_RX_SYNC);

    unsigned int bitClock = samplingFrequency * 16 * 2;
    PRCMI2SClockFreqSet(bitClock);
    I2SConfigSetExpClk(base, bitClock, bitClock, I2S_MODE_MASTER | I2S_SLOT_SIZE_16 | I2S_PORT_CPU);

    I2SSerializerConfig(base, I2S_DATA_LINE_0, I2S_SER_MODE_TX, I2S_INACT_LOW_LEVEL);
    I2SSerializerConfig(base, I2S_DATA_LINE_1, I2S_SER_MODE_RX, I2S_INACT_LOW_LEVEL);

    // Configure I2S pins in pin mux
    PinTypeI2S(PIN_64, PIN_MODE_7); // xr0Pin = I2S SDout (CC3220S-LAUNCHXL) = DIN_J3 (CC3200AUDBOOST)
    PinTypeI2S(PIN_50, PIN_MODE_6); // xr1Pin = I2S SDin (CC3220S-LAUNCHXL) = DOUT_J3 (CC3200AUDBOOST)
    PinTypeI2S(PIN_53, PIN_MODE_2); // clkPin = I2S SCLK (CC3220S-LAUNCHXL) = BCLK_J3 (CC3200AUDBOOST)
    PinTypeI2S(PIN_63, PIN_MODE_7); // fsxPin = I2S WC (CC3220S-LAUNCHXL) = FSYNC_J3 (CC3200AUDBOOST)

    PRCMPeripheralClkEnable(peripheral, PRCM_RUN_MODE_CLK);
}

#define CODEC_I2C_SLAVE_ADDR ((0x30 >> 1))

static uint8_t CodecRegRead(I2C_Handle i2cHandle, uint8_t regAddr)
{
    I2C_Transaction i2cTransaction;
    uint8_t data;

    i2cTransaction.slaveAddress = CODEC_I2C_SLAVE_ADDR;
    i2cTransaction.writeBuf =  &regAddr;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = &data;
    i2cTransaction.readCount = 1;
    if (!I2C_transfer(i2cHandle, &i2cTransaction))
    {
        // I2C transfer failed
        while (1);
    }
    return data;
}

static void CodecRegWrite(I2C_Handle i2cHandle, uint8_t regAddr, uint8_t regValue)
{
    uint8_t data[2];
    I2C_Transaction i2cTransaction;

    data[0] = regAddr;
    data[1] = regValue;

    i2cTransaction.slaveAddress = CODEC_I2C_SLAVE_ADDR;
    i2cTransaction.writeBuf =  &data[0];
    i2cTransaction.writeCount = 2;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;
    if (!I2C_transfer(i2cHandle, &i2cTransaction))
    {
        // I2C transfer failed
        while (1);
    }
}

static void CodecPageSelect(I2C_Handle i2cHandle, unsigned long pageAddress)
{
    CodecRegWrite(i2cHandle, 0, pageAddress);
}

static void CodecReset(I2C_Handle i2cHandle)
{
    // Select page 0.
    CodecPageSelect(i2cHandle, 0);
    // Soft RESET.
    CodecRegWrite(i2cHandle, 1, 0x01);
    // Wait for 27000 * 3 = 81000 clock cycles @ 80 MHz ~ 1 ms.
    UtilsDelay(27000);
}

// volume: 0 -> 0 bB (Highest) to 116 -> -72.3 dB (Lowest)
void AudioVolumeControl(I2C_Handle i2cHandle, signed char volume)
{
    // Select page 1
    CodecPageSelect(i2cHandle, 1);
    // Enable HPL output analog volume
    CodecRegWrite(i2cHandle, 22, volume);
    CodecRegWrite(i2cHandle, 23, volume);
}

void CodecMute(I2C_Handle i2cHandle)
{
    // Select page 0.
    CodecPageSelect(i2cHandle, 0);
    // Mute.
    CodecRegWrite(i2cHandle, 64, 0x0C);
}

void CodecUnmute(I2C_Handle i2cHandle)
{
    // Select page 0.
    CodecPageSelect(i2cHandle, 0);
    // Unmute.
    CodecRegWrite(i2cHandle, 64, 0x00);
}

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

void ConfigureAudioCodec(I2C_Handle i2cHandle, unsigned int samplingFrequency)
{
    // Check parameter.
    if (samplingFrequency < 8000 || samplingFrequency > 48000 || samplingFrequency % 4000 != 0)
    {
        while(1);
        // Wrong value for sampling frequency.
    }
    size_t sampleIndex = (samplingFrequency / 4000) - 2;
    // values for DOSR, MDAC, NDAC, NADC and J in steps of 4 kHz starting from 8 kHz to 48 kHz.
    //              8   12   16   20   24   28   32   36   40   44   48
    int DOSR[] = {512, 512, 384, 304, 256, 208, 192, 160, 144, 128, 128};
    int MDAC[] = {  1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2};
    int NDAC[] = {  1,   1,   2,  16,   8,   8,   8,   8,   8,   8,   8};
    int NADC[] = {  2,   2,   3,  19,  16,  13,  12,  10,   9,   8,   8};
    int J[]    = {  4,   4,   6,  38,  32,  26,  24,  20,  18,  16,  16};

    // Reset code for startup.
    CodecReset(i2cHandle);

    // Select page 0.
    CodecPageSelect(i2cHandle, 0);

    // Set I2S Mode and Word Length = 16 bits, BCLK and WCLK are inputs to the device.
    CodecRegWrite(i2cHandle, 27, 0x00);
    // Clock settings chip
    CodecRegWrite(i2cHandle, 4, 0x03); // PLL CLock is CODEC_CLKIN
    CodecRegWrite(i2cHandle, 5, 0x94); // PLL enabled, P = 1, R = 4
    CodecRegWrite(i2cHandle, 6, J[sampleIndex]);   // PLL J
    CodecRegWrite(i2cHandle, 7, 0);    // PLL D = 0
    CodecRegWrite(i2cHandle, 8, 0);    // PLL D = 0
    // PLL_CLK = PLL_CLKIN * R * J.D / P = PLL_CLKIN * 4 * J = (Fs * 32) * 4 * J

    // Clock settings DAC.
    CodecRegWrite(i2cHandle, 11, 0x80 + NDAC[sampleIndex]); // NDAC is powered up
    CodecRegWrite(i2cHandle, 12, 0x80 + MDAC[sampleIndex]); // MDAC is powered up
    CodecRegWrite(i2cHandle, 13, DOSR[sampleIndex] / 256); // DOSR
    CodecRegWrite(i2cHandle, 14, DOSR[sampleIndex] % 256);
    // DAC_fs = CODEC_CLKIN / (NDAC * MDAC * DOSR)

    // Clock settings ADC.
    CodecRegWrite(i2cHandle, 18, 0x80 + NADC[sampleIndex]); // NADC is powered up, NADC = 2
    CodecRegWrite(i2cHandle, 19, 0x80 + 2); // MADC is powered up, MADC = 2
    CodecRegWrite(i2cHandle, 20, 128); // AOSR = 128
    // ADC_fs = CODEC_CLKIN / (NADC * MADC * AOSR)

    // Configure power supplies.
    CodecPageSelect(i2cHandle, 1);
    CodecRegWrite(i2cHandle, 1, 0x08); // AVdd and DVdd are connected
    CodecRegWrite(i2cHandle, 2, 0x01); // LDO enabled AVDD LDO output = 1.72 V
    CodecRegWrite(i2cHandle, 71, 0x32); // Aanalog input powerup = 6.4 ms
    CodecRegWrite(i2cHandle, 123, 0x01); // Reference powered up in 40 ms

    // Configure ADC channel.
    // Route IN1L to Left MICPGA with 10K input impedance.
    CodecRegWrite(i2cHandle, 52, 0x40);
    // Route CM to Left MICPGA with 10K input impedance.
    CodecRegWrite(i2cHandle, 54, 0x40);
    // Route IN1R to Right MICPGA with 10K input impedance.
    CodecRegWrite(i2cHandle, 55, 0x40);
    // Route CM to Right MICPGA with 10K input impedance.
    CodecRegWrite(i2cHandle, 57,0x40);
    // Floating IN1L.
    CodecRegWrite(i2cHandle, 58, 0xC0);

    // Select Page 0.
    CodecPageSelect(i2cHandle, 0);
    // Power up LADC/RADC.
    CodecRegWrite(i2cHandle, 81, 0xC0);
    // Unmute LADC/RADC.
    CodecRegWrite(i2cHandle, 82, 0x00);

    // Configure DAC channel.
    // Select Page 1.
    CodecPageSelect(i2cHandle, 1);

    // De-pop: soft stepping disabled, N = 5, Rpop = 6k. See SLAA408A page 11,12,13.
    CodecRegWrite(i2cHandle, 20, 0x25);

    // Route LDAC/RDAC to HPL/HPR.
    CodecRegWrite(i2cHandle, 12, 0x08);
    CodecRegWrite(i2cHandle, 13, 0x08);

    // Power up HPL/HPR drivers.
    CodecRegWrite(i2cHandle, 9, 0x30);

    // Unmute HPL/HPR driver, 0dB Gain.
    CodecRegWrite(i2cHandle, 16, 0x00);
    CodecRegWrite(i2cHandle, 17, 0x00);

    // Select Page 0.
    CodecPageSelect(i2cHandle, 0);

    // Unmute DAC, 0dB Gain.
    CodecRegWrite(i2cHandle, 65, 0x00);
    CodecRegWrite(i2cHandle, 66, 0x00);

    // Select Page 1.
    CodecPageSelect(i2cHandle, 1);

    while (CodecRegRead(i2cHandle, 63) & 0x11000000 != 0x11000000)
    {
        UtilsDelay(27000); // delay 27000 * 3 = 81000 clock cycles @ 80 MHz ~ 1 ms.
    }

    // Select Page 0.
    CodecPageSelect(i2cHandle, 0);

    // Power up LDAC/RDAC.
    CodecRegWrite(i2cHandle, 63, 0xd4);

    // Unmute LDAC/RDAC.
    CodecRegWrite(i2cHandle, 64, 0x00);
}

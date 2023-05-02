/*

	Transform library: FFT transform example
	Copyright (C) 2023 Lucio Rossi

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

/*
  Some basic operations with signals
*/

#include "Transform.h"

/*
These values can be changed in order to evaluate the functions
*/
const uint16_t samples = 512; //This value MUST ALWAYS be a power of 2
const double signalFrequency = 500;
const double samplingFrequency = 5000;
const int32_t amplitude = 512;
/*
These are the input and output vectors
Input vectors receive computed results from FFT
*/
int32_t real[samples];
int32_t imag[samples];
uint32_t module[samples>>1];

Transform transformer;

void setup()
{
    Serial.begin(115200);
    while(!Serial);
    Serial.println("Ready");
    transformer.debug(Serial);
}

void loop()
{
    /* Build raw data */

    for (uint16_t i = 0; i < samples; i++)
    {
        int32_t angle = int32_t(TWOPI_DIVISIONS*((i*signalFrequency)/samplingFrequency));
        real[i] = int32_t(transformer.approx_cos_proj(amplitude, angle));
        imag[i] = 0;
    }

    IntSignal signal(real, imag, samples);

    Serial.println("Before FFT");
    transformer.printSignal(signal);

    Serial.println("--");
    unsigned long time_delta = millis();
    transformer.FFT(signal);
    time_delta = millis() - time_delta;

    signal.getSignalModule(module);
    uint16_t f_peak_index = getMaxIndex(module, samples>>1);

    float signal_peak_freq = f_peak_index*(samplingFrequency/samples);

    Serial.println("After FFT");
    transformer.printSignal(signal);

    Serial.print("module=[");
    for (uint16_t i=0; i< samples>>1; i++) {
        Serial.print(module[i]);
        Serial.print(", ");
    }
    Serial.println("]");

    Serial.print("found peak @(Hz): ");
    Serial.println(signal_peak_freq);

    Serial.print("FFT calculated in (ms): ");
    Serial.println(time_delta);

    delay(9999); /* Repeat after delay */
}
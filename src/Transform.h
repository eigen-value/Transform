/*

	Transform library
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

#ifndef Transform_H
#define Transform_H

/* arcsin lookup table for the fast, binary section, sin and cos approximate algorithms. This table must 
be 2^TRIG_ACCURACY_MAX (TRIG_UNITY) in size and the elements should span from 0 to TWOPI_DIVISIONS/4.
Ex. if TRIG_ACCURACY_MAX is 7 the unity is divided into 128 intervals and if 
TWO_PI_DIVISIONS is 1024 then 0-pi/2 must correspond to 0-256 (roughly)
 */
static const byte arcsin_data[128] =
{ 0,  1,   3,   4,   5,   6,   8,   9,   10,  11,  13,  14,  15,  17,  18,  19,  20,
  22,  23,  24,  26,  27,  28,  29,  31,  32,  33,  35,  36,  37,  39,  40,  41,  42,
  44,  45,  46,  48,  49,  50,  52,  53,  54,  56,  57,  59,  60,  61,  63,  64,  65,
  67,  68,  70,  71,  72,  74,  75,  77,  78,  80,  81,  82,  84,  85,  87,  88,  90,
  91,  93,  94,  96,  97,  99,  100, 102, 104, 105, 107, 108, 110, 112, 113, 115, 117,
  118, 120, 122, 124, 125, 127, 129, 131, 133, 134, 136, 138, 140, 142, 144, 146, 148,
  150, 152, 155, 157, 159, 161, 164, 166, 169, 171, 174, 176, 179, 182, 185, 188, 191,
  195, 198, 202, 206, 210, 215, 221, 227, 236
};

enum class FFTDirection { Reverse, Forward };
#define FFT_FORWARD FFTDirection::Forward
#define FFT_REVERSE FFTDirection::Reverse

#include <Arduino.h>

#define TRIG_ACCURACY_MAX 7
#define TRIG_UNITY 128				// must be = 2^TRIG_ACCURACY_MAX
#define SIGNAL_LEN_MAX 1024			// must be = TWOPI_DIVISIONS
#define TWOPI_DIVISIONS 1024
#define PI_DIVISIONS 512
#define HALFPI_DIVISIONS 256
#define THREEHALFPI_DIVISIONS 768
#define HALFPI_DIVISIONS_LOG2 8 	// used for finding quadrant
#define FFT_LIMIT_SWING 1024			// minimum signal swing to get best FFT precision
#define FFT_LIMIT_SWING_LOG2 10

class IntSignal {
	
	public:
	IntSignal(int32_t* real_in, int32_t* imag_in, uint16_t samples);
	~IntSignal();
	int32_t* real = nullptr;
	int32_t* imag = nullptr;
	void scale2(int8_t pow);
	int32_t get__avg();
	int32_t remove_avg();
	uint16_t getSamples();

	private:
	uint16_t _samples;
	int16_t _scale_pow = 0;		// !=0 if signal is scaled by a power of 2
	int32_t _avg = 0;			// stored average (real). !=0 if signal avg has been removed
	
};

class Transform {
	
	public:
	Transform();
	~Transform();

	void debug(Stream&);
	void FFT(IntSignal& signal, uint8_t accuracy=5, FFTDirection dir=FFT_FORWARD);
	uint16_t log2(uint16_t n);
	int32_t approx_sin_proj(int32_t A, int32_t theta_divs, uint8_t accuracy=5);
	int32_t approx_cos_proj(int32_t A, int32_t theta_divs, uint8_t accuracy=5);
	int32_t unwrap(int32_t theta_divs);
	uint32_t swing(int32_t* v, uint16_t samples);
	void InverseBit(int32_t* v, uint16_t samples);
	void printSignal(IntSignal& signal);
	
	private:
	uint32_t reverse_bits(uint32_t x, int bits);
	Stream *_debug = nullptr;
	
};

#endif
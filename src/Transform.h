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

#include <Arduino.h>

class IntSignal {
	
	public:
	IntSignal(int32_t* real_in, int32_t* imag_in, uint16_t samples);
	~IntSignal();
	int32_t* real = nullptr;
	int32_t* imag = nullptr;
	void scale2(int8_t pow);
	uint16_t getSamples();

	private:
	uint16_t _samples;
	int16_t _scale_pow = 0;		// !=0 if signal is scaled by a power of 2
	int32_t _avg = 0;			// stored average. !=0 if signal avg has been removed
	
};

class Transform {
	
	public:
	Transform();
	~Transform();

	void debug(Stream&);
	// void FFT(IntSignal& signal);
	uint16_t log2(uint16_t n);
	void InverseBit(int32_t* v, uint16_t size);
	void printSignal(IntSignal& signal);
	
	private:
	uint32_t reverse_bits(uint32_t x, int bits);
	Stream *_debug = nullptr;
	
};

#endif
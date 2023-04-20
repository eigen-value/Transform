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

#include "Arduino.h"
#include "Transform.h"


IntSignal::IntSignal(int32_t* real_in, int32_t* imag_in, uint16_t samples) {
	
	_samples = samples;
	real = real_in;
	imag = imag_in;

}

IntSignal::~IntSignal(void) {
  // Destructor
}

void IntSignal::scale2(int8_t pow) {
	
	_scale_pow += pow;
	
	if (pow < 0) {
		for (uint16_t i=0; i<_samples; i++) {
			real[i] = real[i]>>-pow;
		}
	} else if (pow > 0) {
		for (uint16_t i=0; i<_samples; i++) {
			real[i] = real[i]<<pow;
		}
	}
	
}


uint16_t IntSignal::getSamples() {
	return _samples;
}

/* Transform class*/

Transform::Transform(void) {
  // Constructor
}

Transform::~Transform(void) {
  // Destructor
}

uint16_t Transform::log2(uint16_t n) {

	uint16_t out = 0;
	while (n>1) {
		n>>=1;
		out++;
	}

	return out;

}

uint32_t Transform::reverse_bits(uint32_t x, int bits) {
	x = ((x & 0x55555555) << 1) | ((x & 0xAAAAAAAA) >> 1); // Swap _<>_
    x = ((x & 0x33333333) << 2) | ((x & 0xCCCCCCCC) >> 2); // Swap __<>__
    x = ((x & 0x0F0F0F0F) << 4) | ((x & 0xF0F0F0F0) >> 4); // Swap ____<>____
    x = ((x & 0x00FF00FF) << 8) | ((x & 0xFF00FF00) >> 8); // Swap ...
    x = ((x & 0x0000FFFF) << 16) | ((x & 0xFFFF0000) >> 16); // Swap ...
    return x >> (32 - bits);
}

void Transform::InverseBit(int32_t* v, uint16_t size) {
	
	for (uint16_t i=0; i<size; i++) {
		_debug->print(v[i]);
		_debug->print(",");
	}
	_debug->println();

	for (uint16_t i=0; i<size>>1; i++) {
		int32_t temp = v[i];
		uint16_t i_reverse = reverse_bits(i, log2(size));
		v[i] = v[i_reverse];
		v[i_reverse] = temp;
	}

	for (uint16_t i=0; i<size; i++) {
		_debug->print(v[i]);
		_debug->print(",");
	}
	_debug->println();

}

void Transform::printSignal(IntSignal& signal) {
	
	_debug->println("real | imag");
	for (uint16_t i=0; i<signal.getSamples(); i++) {
		
		_debug->print(signal.real[i]);
		_debug->print(" | ");
		_debug->println(signal.imag[i]);
		
	}
	
}


void Transform::debug(Stream& stream) {
	_debug = &stream;
}
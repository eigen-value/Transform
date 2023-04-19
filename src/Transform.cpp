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
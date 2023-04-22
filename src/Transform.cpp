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
	
	int log2_size = log2(size);

	for (uint16_t i=0; i<size>>1; i++) {
		int32_t temp = v[i];
		uint16_t i_reverse = reverse_bits(i, log2_size);
		v[i] = v[i_reverse];
		v[i_reverse] = temp;
	}

}

void Transform::printSignal(IntSignal& signal) {
	
	_debug->println("real | imag");
	for (uint16_t i=0; i<signal.getSamples(); i++) {
		
		_debug->print(signal.real[i]);
		_debug->print(" | ");
		_debug->println(signal.imag[i]);
		
	}
	
}

int32_t Transform::approx_sin_proj(int32_t A, int32_t theta_divs, uint8_t accuracy) {

	if (theta_divs == 0 || theta_divs == PI_DIVISIONS || theta_divs == TWOPI_DIVISIONS) {return 0;}
	if (theta_divs == HALFPI_DIVISIONS) {return A;}
	if (theta_divs == THREEHALFPI_DIVISIONS) {return -A;}

	if (accuracy > TRIG_ACCURACY_MAX) {
		accuracy = TRIG_ACCURACY_MAX;
		_debug->print("Transform::approx_sin_proj accuracy cannot be greater than ");
		_debug->println(TRIG_ACCURACY_MAX);
	}


	// unwinding for lesser-than-0 bigger-than-2pi theta angles
	theta_divs = unwind(theta_divs);


	// Everything can be reduced to the 1st quadrant case (0-pi/2)
	int32_t quad = theta_divs;
	while(quad>3) {		// theta quadrant
		quad>>=1;
	}
	// bring theta to quad 0
	if (quad == 1) {
	theta_divs = PI_DIVISIONS - theta_divs;
	}
	else if (quad == 2) {
	theta_divs = theta_divs - PI_DIVISIONS;
	}
	else if (quad == 3) {
	theta_divs = PI_DIVISIONS - theta_divs;
	}


	// It's a binary search in the 0-1 interval, corresponding to 0-A
	byte sin_guess = TRIG_UNITY >> 1;			// starting the guess from the middle: 1/2
	byte sin_step = TRIG_UNITY >> 2;			// approx step starting from 1/4

	int32_t Asin_guess = A >> 1;				// starting the guess from the middle: A/2
	int32_t Asin_step = A >> 2;					// approximation step starting from A/4

	for (int i = 0; i < accuracy; i++)
	{ 

		if (theta_divs > arcsin_data[sin_guess]) {
			sin_guess += sin_step;
			Asin_guess += Asin_step;
		} else if (theta_divs < arcsin_data[sin_guess]) {
			sin_guess -= sin_step;
			Asin_guess -= Asin_step;
		} else {	// we have the right guess so the loop can be broken
			break;
		}

		sin_step>>=1;
		Asin_step>>=1;
	}

	// Adjust for the right quadrant
	if (quad == 2 || quad == 3) {
		return -Asin_guess;
	}

	return Asin_guess;
}

int32_t Transform::approx_cos_proj(int32_t A, int32_t theta_divs, uint8_t accuracy) {
	theta_divs = TWOPI_DIVISIONS>>2 - theta_divs;		// cos(x) = sin(90-x)
}

void Transform::FFT(IntSignal& signal, uint8_t accuracy) {
	/* Scaling */

	/* Bit reversal*/
	InverseBit(signal.real, signal.getSamples());
	// if ifft do the same on .imag

	/* Butterfly products*/


}

int32_t Transform::unwind(int32_t theta_divs) {
	while (theta_divs>TWOPI_DIVISIONS) {
		theta_divs -= TWOPI_DIVISIONS;
	}

	while (theta_divs<0) {
		theta_divs += TWOPI_DIVISIONS;
	}
}

void Transform::debug(Stream& stream) {
	_debug = &stream;
}
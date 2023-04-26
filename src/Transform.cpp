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
	
	_debug->print("real = [");
	for (uint16_t i=0; i<signal.getSamples(); i++) {
		
		_debug->print(signal.real[i]);
		_debug->print(", ");
		
	}
	_debug->println("]");

	_debug->print("imag = [");
	for (uint16_t i=0; i<signal.getSamples(); i++) {
		
		_debug->print(signal.imag[i]);
		_debug->print(", ");
		
	}
	_debug->println("]");
	
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

	// unwrapping for lesser-than-0 bigger-than-2pi theta angles
	theta_divs = unwrap(theta_divs);

	// Everything can be reduced to the 1st quadrant case (0-pi/2)
	int32_t quad = theta_divs>>HALFPI_DIVISIONS_LOG2; // theta quadrant

	// bring theta to quad 0
	if (quad == 1) {
		theta_divs = PI_DIVISIONS - theta_divs;
	}
	else if (quad == 2) {
		theta_divs = theta_divs - PI_DIVISIONS;
	}
	else if (quad == 3) {
		theta_divs = TWOPI_DIVISIONS - theta_divs;
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
	return approx_sin_proj(A, HALFPI_DIVISIONS - theta_divs, accuracy);		// cos(x) = sin(90-x)
}

void Transform::FFT(IntSignal& signal, uint8_t accuracy, FFTDirection dir) {


	uint16_t samples = signal.getSamples();
	uint16_t log2samples = log2(samples);

	/* Scaling */

	/* Bit reversal*/
	InverseBit(signal.real, signal.getSamples());
	// if ifft do the same on .imag

	/* FFT Butterfly products*/
	uint16_t angle_span = PI_DIVISIONS;  // angle span to calculate all the N-radices of 1 (weights). It halves every cycle 180°, 90°, 45°...
	uint16_t l2 = 1;
	for (uint8_t loop = 0; (loop < log2samples); loop++) {  // in-place fft is made of log2(samples) steps
		uint16_t l1 = l2;   // l1 is the butterfly span. It doubles every cycle l1 = 1,2,4,8... It is also the number of butterfly groups (same W)
		l2 <<= 1;           // l2 is the distance between one butterfly and the next with same weight. doubles every cycle l2 = 2,4,8...
		// double u1 = 1.0;
		// double u2 = 0.0;
		uint16_t theta = TWOPI_DIVISIONS;
		for (uint16_t j = 0; j < l1; j++) { // butterfly-group cycle
			for (uint16_t i = j; i < samples; i += l2) { // calculate butterflies with same weight (l2 distant)
			uint16_t i1 = i + l1;
			// double t1 = u1 * signal.real[i1] - u2 * signal.imag[i1];
			// double t2 = u1 * signal.imag[i1] + u2 * signal.real[i1];

			int32_t t1;
			int32_t t2;
			t1 = approx_cos_proj(signal.real[i1], theta, accuracy) - approx_sin_proj(signal.imag[i1], theta, accuracy);
			t2 = approx_cos_proj(signal.imag[i1], theta, accuracy) + approx_sin_proj(signal.real[i1], theta, accuracy);
			signal.real[i1] = signal.real[i] - t1;
			signal.imag[i1] = signal.imag[i] - t2;
			signal.real[i] += t1;
			signal.imag[i] += t2;
			}
			// application of rotation matrix [cos(th), -sin(th); sin(th), cos(th)] to get the next N-radix of 1
			// double z = ((u1 * c1) - (u2 * c2));
			// u2 = ((u1 * c2) + (u2 * c1));
			// u1 = z;
			if (dir == FFT_FORWARD) {
			theta = theta-angle_span;
			} else {
			theta = theta+angle_span;
			}
		}

		// halving the angle at every step
		// c2 = sqrt((1.0 - c1) / 2.0);    // sin(theta/2)
		// c1 = sqrt((1.0 + c1) / 2.0);    // cos(theta/2)
		angle_span>>1;
		// if (dir == FFT_FORWARD) {
		//   c2 = -c2;   // remember W exponent is -j2pi/N
		// }
	}

	// Scaling for reverse transform /
	if (dir != FFT_FORWARD) {
		for (uint16_t i = 0; i < samples; i++) {
			signal.real[i] /= samples;
			signal.imag[i] /= samples;
		}
	}

}

int32_t Transform::unwrap(int32_t theta_divs) {

	while (theta_divs>TWOPI_DIVISIONS) {
		theta_divs -= TWOPI_DIVISIONS;
	}

	while (theta_divs<0) {
		theta_divs += TWOPI_DIVISIONS;
	}

	return theta_divs;

}

void Transform::debug(Stream& stream) {
	_debug = &stream;
}
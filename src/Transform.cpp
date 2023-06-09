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
			imag[i] = imag[i]>>-pow;
		}
		_avg = _avg>>-pow;
	} else if (pow > 0) {
		for (uint16_t i=0; i<_samples; i++) {
			real[i] = real[i]<<pow;
			imag[i] = imag[i]<<pow;
		}
		_avg = _avg<<pow;
	}
	
}

int32_t IntSignal::get__avg() {
	return _avg;
}

int32_t IntSignal::remove_avg() {

	for (uint16_t i=0; i<_samples; i++) {
		_avg+=real[i];
	}

	_avg/=_samples;

	for (uint16_t i=0; i<_samples; i++) {
		real[i]-=_avg;
	}

	return _avg;

}

uint16_t IntSignal::getSamples() {
	return _samples;
}

void IntSignal::getSignalModule(uint32_t* module) {

	for (uint16_t i=0; i<(_samples>>1); i++) {
		module[i] = approx_module(real[i], imag[i]);
	}

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

void Transform::InverseBit(int32_t* v, uint16_t samples) {
	
	int log2_samples = log2(samples);

	for (uint32_t i=0; i<samples; i++) {
		int32_t temp = v[i];
		uint32_t i_reverse = reverse_bits(i, log2_samples);
		if (i>=i_reverse) {continue;} 
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

	if (accuracy > TRIG_ACCURACY_MAX) {
		accuracy = TRIG_ACCURACY_MAX;
		_debug->print("Transform::approx_sin_proj accuracy cannot be greater than ");
		_debug->println(TRIG_ACCURACY_MAX);
	}

	// unwrapping for lesser-than-0 bigger-than-2pi theta angles
	theta_divs = unwrap(theta_divs);

	if (theta_divs == 0 || theta_divs == PI_DIVISIONS || theta_divs == TWOPI_DIVISIONS) {return 0;}
	if (theta_divs == HALFPI_DIVISIONS) {return A;}
	if (theta_divs == THREEHALFPI_DIVISIONS) {return -A;}

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

	/* Scaling only for FFT_FORWARD */
	if (dir == FFT_FORWARD) {
		uint32_t real_swing = swing(signal.real, samples);
		if (real_swing<FFT_LIMIT_SWING) {
			signal.remove_avg();
			signal.scale2(FFT_LIMIT_SWING_LOG2-log2(real_swing));
		}
	}

	/* Bit reversal */
	InverseBit(signal.real, samples);
	if (dir == FFT_REVERSE) {
		InverseBit(signal.imag, samples);
	}

	/* FFT Butterfly products */
	uint16_t angle_span = PI_DIVISIONS;  // angle span to calculate all the N-radices of 1 (weights). It halves every cycle 180°, 90°, 45°...
	uint16_t l2 = 1;
	for (uint8_t loop = 0; (loop < log2samples); loop++) {  // in-place fft is made of log2(samples) steps
		uint16_t l1 = l2;   // l1 is the butterfly span. It doubles every cycle l1 = 1,2,4,8... It is also the number of butterfly groups (same W)
		l2 <<= 1;           // l2 is the distance between one butterfly and the next with same weight. doubles every cycle l2 = 2,4,8...

		uint16_t theta = TWOPI_DIVISIONS;
		for (uint16_t j = 0; j < l1; j++) { // butterfly-group cycle
			for (uint16_t i = j; i < samples; i += l2) { // calculate butterflies with same weight (l2 distant)
				uint16_t i1 = i + l1;
				// calculating the butterfly products modifiers
				int32_t t1;
				int32_t t2;
				t1 = approx_cos_proj(signal.real[i1], theta, accuracy) - approx_sin_proj(signal.imag[i1], theta, accuracy);
				t2 = approx_cos_proj(signal.imag[i1], theta, accuracy) + approx_sin_proj(signal.real[i1], theta, accuracy);
				signal.real[i1] = signal.real[i] - t1;
				signal.imag[i1] = signal.imag[i] - t2;
				signal.real[i] += t1;
				signal.imag[i] += t2;
			}
			// rotation of theta
			if (dir == FFT_FORWARD) {
				theta = theta-angle_span;
			} else {
				theta = theta+angle_span;
			}
		}

		// halving the angle span at every step
		angle_span>>=1;

	}

	// Scaling for reverse transform /
	if (dir == FFT_REVERSE) {
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

uint32_t Transform::swing(int32_t* v, uint16_t samples) {

	int32_t max = v[0];
	int32_t min = v[0];

	for (uint16_t i=1; i<samples; i++) {
		if (v[i] > max) { max = v[i];}
		if (v[i] < min) { min = v[i];}
	}

	return max-min;

}

void Transform::debug(Stream& stream) {
	_debug = &stream;
}

uint16_t getMaxIndex(uint32_t* v, uint16_t samples) {

	uint16_t max_index = 0;
	uint32_t max = v[0];

	for (uint16_t i=1; i<samples; i++) {

		if (v[i] > max) {
			max_index = i;
			max = v[i];
		}

	}

	return max_index;

}

float getApproxMaxPosition(uint32_t* v, uint16_t samples) {
	/*
	returns the position as a weighted average of three samples (max and 1st two neighbors)
	*/

	uint16_t idx = getMaxIndex(v, samples);

	if (idx==0 || idx==samples-1) {return float(idx);}

	float a = v[idx-1];
	float b = v[idx];
	float c = v[idx+1];

	return (a * (idx - 1) + b * idx + c * (idx + 1)) / (a + b + c);

}

uint32_t approx_module(int32_t a, int32_t b) {
	// Works best for numbers > 16

	if (a==0 && b == 0) {return 0;}

	if (a<0) {a=-a;}
	if (b<0) {b=-b;}

	uint32_t max;
	uint32_t min;

	if (a>b) {
		max = a;
		min = b;
	} else {
		max = b;
		min = a;
	}

	if (max > min+min+min)		// if max>3min assuming module = |max| has less than 5% error
	{
		return max;
	} else {
		uint8_t multiple = 0;
		uint32_t min_over8 = min>>3;
		if (min_over8==0) {min_over8 = 1;}
		uint32_t min_over16 = min_over8>>1;

		uint32_t temp = min;
		while (temp<max){
			temp += min_over8;
			multiple++;
		}

		for (int i = 0; i < approx_module_cycles[multiple]; i++) {	// adding 'multple' times min/16 to adjust approximation
			max = max + min_over16;
		}

		return max;

	}

}
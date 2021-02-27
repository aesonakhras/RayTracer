#pragma once
#include <stdint.h>

namespace CV {
	//Be careful 
	typedef struct ColorBMP {
		uint8_t b;
		uint8_t g;
		uint8_t r;
		ColorBMP(uint8_t inR, uint8_t inG, uint8_t inB) :
			r(inR),
			g(inG),
			b(inB) {};

		ColorBMP operator+=(const ColorBMP& other) {
			this->r += other.r;
			this->b += other.b;
			this->g += other.g;
			return *this;
		}

		ColorBMP operator*(const float& other) {
			this->r = (uint8_t)(other * (float)r);
			this->g = (uint8_t)(other * (float)g);
			this->b = (uint8_t)(other * (float)b);
			
			return *this;
		}

		ColorBMP(const ColorBMP&) = default;
		ColorBMP& operator=(const ColorBMP&) = default;
	} ColorBMP;
}
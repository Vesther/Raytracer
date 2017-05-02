// Color utility class with 8 bits of depth per color

#include <SFML/Graphics.hpp>
#include <stdint.h>
#pragma once

class Color
{
public:
	uint8_t r, g, b;

	// Initializes to Black by default
	Color() : r(0), g(0), b(0) {}
	Color(uint8_t rr, uint8_t gg, uint8_t bb) : r(rr), g(gg), b(bb) {}

	// Conversion to sf::Color
	operator sf::Color();

	Color operator*(const Color& right) const
	{
		return Color((int)r*(int)right.r / 255, (int)g*(int)right.g / 255, (int)b*(int)right.b / 255);
	}

	Color operator*(const float right) const
	{
		return Color(r * 255 * right / 255, g * 255 * right / 255, b * 255 * right / 255);
	}

	Color operator+(Color& right)
	{
		return Color(std::min(r + right.r, 255), std::min(g + right.g, 255), std::min(b + right.b, 255));
	}

	// Static color constants
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Black;
	static const Color White;
	static const Color Cyan;
	static const Color Magenta;
	static const Color Yellow;
};
// Color utility class with 8 bits of depth per color

#include <SFML/Graphics.hpp>
#include <stdint.h>

class Color
{
public:
	uint8_t r, g, b;

	// Initializes to Black by default
	Color() : r(0), g(0), b(0) {}
	Color(uint8_t rr, uint8_t gg, uint8_t bb) : r(rr), g(gg), b(bb) {}

	operator sf::Color();

	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Black;
	static const Color White;
};
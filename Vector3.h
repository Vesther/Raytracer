// A class template implementation of a 3D vector

/*
	Note:
	SFML actually implements a subset of this functionality with the sf::Vector3<T> Class Template.
	Since using that would take away a lot of fun, i'm writing my own implementation here.
*/

#include <math.h> // For sqrt() in the length() method
#include <iostream>

template <class T> class Vector3
{
public:
	// Vector components
	T x, y, z;


	// Default constructor for 0 arguments given
	Vector3() : x(0), y(0), z(0) {}
	// Constructor for 3 Arguments
	Vector3(T xx, T yy, T zz) : x(xx), y(yy), z(zz) {}


	// -- Methods --
	// Returns the length of the vector
	T length()
	{
		return sqrt(x*x + y*y + z*z);
	}

	// Normalizes the vector (modifies the vector it's called on)
	Vector3<T> normalize()
	{
		T len = this->length();
		x = x / len;
		y = y / len;
		z = z / len;
		return *this;
	}
	
	// Returns the dot product of 2 vectors
	T dot(const Vector3<T> right)
	{
		return x * right.x + y * right.y + z * right.z;
	}

	// Operator overloads
	Vector3<T> operator+ (const Vector3<T> right)
	{
		return Vector3<T>(x + right.x, y + right.y, z + right.z);
	}

	Vector3<T> operator- (const Vector3<T> right)
	{
		return Vector3<T>(x - right.x, y - right.y, z - right.z);
	}

	Vector3<T> operator* (const Vector3<T> right)
	{
		return Vector3<T>(x * right.x, y * right.y, z * right.z);
	}

	friend std::ostream& operator<<(std::ostream& stream, const Vector3<T>& v)
	{
		stream << "[" << v.x << ", " << v.y << ", " << v.z << "]";
		return stream;
	}

};

// Aliases for common Vector types
typedef Vector3<float> Vector3f;
typedef Vector3<double> Vector3d;
typedef Vector3<int> Vector3i;
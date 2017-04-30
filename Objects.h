#include "stdafx.h"
#include "Color.h"
#include "Vector3.h"
#include "Ray.h"

// Superclass for all 3D Objects that can be rendered
class Object {
public:
	Color color;

	virtual float intersects(const Ray& ray) = 0;
};

class Sphere : public Object
{
public:
	Vector3f center;
	float radius;
	float radius2;

	// Default initializer
	// Sphere() : center(Vector3f()), radius(1.0f), color(Color::Black) {};

	Sphere(Vector3f center, float radius, Color color) : center(center), radius(radius)
	{
		this->color = color;
		radius2 = radius*radius;
	}

	// Check if a Ray intersects this sphere and return the distance to it or -1 if there is no collision
	float intersects(const Ray& ray)
	{
		// Create a line segment between our ray origin and the sphere center
		Vector3f l = center - ray.origin;
		// Using l as the hypotenuse of our triangle, find the length of the adjacent side
		float adj = l.dot(ray.direction);
		// Find the length of the opposite side squared from pythagoras theorem
		float opp = l.dot(l) - (adj*adj);
		// If opp > radius^2, the ray and sphere don't intersect
		if (opp > radius2)
			return -1;

		float thck = sqrt(radius2 - opp);
		float t0 = adj - thck;
		float t1 = adj + thck;

		if (t0 < 0.0f && t1 < 0.0f)
			return -1;

		// If we get to this point, we return the distance
		return (t0 < t1 ? t0 : t1);
	}
};

class Plane : public Object
{
public:
	Vector3f p;
	Vector3f normal;

	Plane(Vector3f p, Vector3f normal, Color color) : p(p), normal(normal) 
	{
		this->color = color;
	}

	float intersects(const Ray& ray)
	{
		float denominator = normal.dot(ray.direction);
		/*
		float distance = -1;
		if (denominator > 1e-6)
		{
			Vector3f v = p - ray.origin;
			distance = v.dot(normal) / denominator;
		}
		*/
		if (denominator > 0.0001)
			return denominator;
		else
			return -1;
	}
};
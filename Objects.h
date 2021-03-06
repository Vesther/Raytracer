#include "stdafx.h"
#include "Color.h"
#include "Vector3.h"
#include "Ray.h"

// Superclass for all 3D Objects that can be rendered
class Object {
public:
	Color color;
	float reflectivity = 1.0f;
	std::string debugID = "UNDEFINED!";

	virtual float intersects(const Ray& ray) = 0;
	virtual Vector3f surface_normal(Vector3f hit_point) = 0;
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

	Vector3f surface_normal(Vector3f hit_point)
	{
		return (hit_point - this->center).normalize();
	}
};


// Not working, too tired and too late to figure out why.
// In theory, if the dot product of a ray's direction vector and a plane's normal vector is anything else than 0, they will eventually intersect.
class Plane : public Object
{
public:
	Vector3f origin;
	Vector3f normal;

	Plane(Vector3f origin, Vector3f normal, Color color) : origin(origin), normal(normal) 
	{
		this->color = color;
	}

	float intersects(const Ray& ray)
	{
		double denom = normal.dot(ray.direction);
		if (denom > 1e-6)
		{
			Vector3f v = this->origin - ray.origin;
			float distance = v.dot(normal);
			distance = distance / denom;
			if (distance >= 0.0f)
			{
				return distance;
			}
			else return -1;
		}
		else return -1;
	}

	Vector3f surface_normal(Vector3f hit_point)
	{
		return normal*-1;
	}
};
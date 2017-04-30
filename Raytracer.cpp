// Main file, defines entry point

#include "stdafx.h"
#include <SFML/Graphics.hpp>
#include "Vector3.h"
#include "Color.h"
#include <vector>
#include <fstream>
#include <stdint.h>

#define M_PI 3.14159265358979323846
const int image_width = 800;
const int image_height = 600;

class Ray
{
public:
	Vector3f origin;
	Vector3f direction;
};

class Sphere {
public:
	Vector3f center;
	float radius;
	Color color;

	Sphere(){};

	// Check if a Ray intersects this sphere
	bool intersects(const Ray& ray)
	{
		// Create a line segment between our ray origin and the sphere center
		Vector3f l = center - ray.origin;
		// Using l as the hypotenuse of our triangle, find the length of the adjacent side
		float adj = l.dot(ray.direction);
		// Find the length of the opposite side squared from pythagoras theorem
		float opp = l.length() * l.length() - (adj*adj);
		// If opp < radius^2, the ray and sphere intersect
		return opp < radius * radius;
	}
};

// Defines the Scene, "World" to be rendered
class Scene {
public:
	int height;
	int width;
	float fov;
	std::vector<Sphere> spheres;
};


// Degree to radian conversion helper
float rad(float degrees)
{
	return (degrees*M_PI) / 180;
}

// Accepts pixel coordinates of the resulting image and returns a directional ray for our "camera" 
Ray create_ray(int x, int y, Scene scene)
{
	// Correct distortion for different aspect ratios
	float aspect_ratio = (float)scene.width / (float)scene.height;

	// Adjust for different FOVs
	float fov_correction = tan(rad(scene.fov) / 2.0f);

	// Translation of image X/Y pixel to World position, our "camera sensor" rectangle in the Scene.
	float sensor_x = (((x + 0.5f) / scene.width) * 2.0f - 1.0f) * aspect_ratio * fov_correction;
	float sensor_y = (1.0f - ((y + 0.5f) / scene.height) * 2.0f) * fov_correction;

	Ray ray;
	// Initialize Origin to [0,0,0], our point of ray casting
	ray.origin = Vector3f();
	// Set the direction vector to a normalized version of our calculated position.
	ray.direction = Vector3f(sensor_x, sensor_y, -1.0f).normalize();

	// debug
	//std::cout << "Ray created at " << ray.origin << " with direction " << ray.direction << "\n";

	return ray;
}

sf::Image render(Scene scene)
{
	sf::Image image;
	image.create(scene.width, scene.height);

	for (int i = 0; i < scene.height; i++)
	{
		for (int j = 0; j < scene.width; j++)
		{
			// This variable holds the resulting color of our trace or black if nothing is hit.
			Color pixel_color;

			Ray ray = create_ray(j, i, scene);
			for (auto sphere : scene.spheres)
			{
				// If we hit something, color it with the hit object's color.
				if (sphere.intersects(ray))
				{
					pixel_color = sphere.color;
				}
				image.setPixel(j, i, pixel_color);
			}
		}
	}

	return image;
}

int main()
{
	// Test scene setup
	Scene test_scene;
	test_scene.fov = 120;
	test_scene.width = image_width;
	test_scene.height = image_height;

	// A sphere to be rendered in the world
	Sphere test_sphere1;
	test_sphere1.center = Vector3f(0, 0, -5.0f);
	test_sphere1.radius = 1.0f;
	test_sphere1.color = Color::Green;

	test_scene.spheres.push_back(test_sphere1);

	sf::Image image = render(test_scene);

	sf::Texture texture;
	texture.loadFromImage(image);
	sf::Sprite sprite;
	sprite.setTexture(texture, true);

	// SFML Window and main loop
	sf::RenderWindow window(sf::VideoMode(image_width, image_height), "Raytracer v0.1");

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		window.clear();
		window.draw(sprite);
		window.display();
	}

	return 0;
}

// Main file, defines entry point

#include "stdafx.h"
#include <SFML/Graphics.hpp>
#include "Vector3.h"
#include "Color.h"
#include "Objects.h"
#include "Ray.h"
#include <vector>
#include <list>
#include <fstream>
#include <stdint.h>
#include <thread>
#include <mutex>
#include <chrono> // Benchmarking
#include <iostream> // Debug messages

#define M_PI 3.14159265358979323846
const int image_width = 800;
const int image_height = 600;

// Set this to true to enable animated rendering, eg. updating of the RenderWindow while it is rendering.
// Note: Very expensive, slows rendering by some 50%;
const bool pretty = false;

// Defines the Scene, "World" to be rendered
class Scene {
public:
	// Height and width of the image resulting from rendering this scene
	int height;
	int width;
	// Field of View parameter
	float fov;
	// Collection of objects to be rendered
	std::vector<Object*> objects;
};

// TODO: Program camera rotation
class Camera {
public:
	Vector3f position;
	Vector3f direction;

	Camera() : position(Vector3f(0,0,0)), direction(Vector3f(0, 0, -1.0f)) { }
};
Camera cam;


// Degree to radian conversion helper
float rad(float degrees)
{
	return (float)((degrees*M_PI) / 180);
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
	// Initialize Origin to cam.position, our point of ray casting
	ray.origin = cam.position;
	// Set the direction vector to a normalized version of our calculated position.
	ray.direction = Vector3f(sensor_x, sensor_y, -1.0f).normalize();

	// debug
	//std::cout << "Ray created at " << ray.origin << " with direction " << ray.direction << "\n";

	return ray;
}

sf::Image image;
sf::Texture texture;
sf::Sprite sprite;

std::mutex render_mutex;

Color trace(Ray &ray, const Scene &scene)
{
	Color pixel_color(16, 16, 16);
	float closest_distance = INT_MAX;
	Object* closest = nullptr;

	for (unsigned int k = 0; k < scene.objects.size(); k++)
	{
		Object& obj = *scene.objects[k];
		// If we hit something, color it with the hit object's color.
		float distance = obj.intersects(ray);
		if (distance == -1) continue;
		if (distance < closest_distance)
		{
			closest_distance = distance;
			closest = &obj;
		}
	}

	if (closest != nullptr)
		pixel_color = closest->color;

	return pixel_color;
}

void render_part(int line_from, int line_to, const Scene &scene, sf::RenderWindow* window)
{
	for (int i = line_from; i < line_to; i++)
	{
		for (int j = 0; j < scene.width; j++)
		{
			Ray ray = create_ray(j, i, scene);
			image.setPixel(j, i, trace(ray, scene));
		}

		// Note: This makes the rendering process "animated" and lets you see the threads at work.
		// It is also stupidly expensive and increases Render time by some 50%.
		if (pretty)
		{
			render_mutex.lock();
			window->setActive(true);
			texture.loadFromImage(image);
			sprite.setTexture(texture, true);
			window->clear();
			window->draw(sprite);
			window->display();
			window->setActive(false);
			render_mutex.unlock();
		}
		// Also, the whole locking and setActive() business is necessary because OpenGL doesn't allow a Context to be active in multiple threads at once. 
	}
}

// The main render function.
// Note: This is multithreaded, it launches several render threads sharing the workload to take advantage of multicore systems.
// Also: Threads != Cores, the Operating System has to worry about Thread->Core allocation, this is just to enable the OS to do so.
sf::Image render(Scene scene, sf::RenderWindow &window)
{
	// Initialize Bitmap
	image.create(scene.width, scene.height, sf::Color::Black);

	// Setup renderthreads
	// Autodetect hardware logical core count
	const int thread_count = std::thread::hardware_concurrency(); 
	//std::cout << thread_count;

	std::list<std::thread*> threads;
	int step = (int)floor(scene.height / thread_count);

	// Launch threads
	for (int i = 0; i < thread_count-1; i++)
	{
		std::thread* renderthread = new std::thread(render_part, i*step, i*step + step, scene, &window);
		threads.push_back(renderthread);
	}
	// Last thread does extra work
	std::thread* renderthread = new std::thread(render_part, (thread_count - 1)*step, scene.height, scene, &window);
	threads.push_back(renderthread);

	// Sync threads
	for (int i = 0; i < thread_count; i++)
	{
		threads.back()->join();
		threads.pop_back();
	}

	window.setActive(true);
	texture.loadFromImage(image);
	sprite.setTexture(texture, true);
	window.clear();
	window.draw(sprite);
	window.display();

	return image;
}

int main()
{
	// Test scene setup
	Scene test_scene;
	test_scene.fov = 90;
	test_scene.width = image_width;
	test_scene.height = image_height;

	// A sphere to be rendered in the world
	Sphere test_sphere1(Vector3f(0, 0, -5.0f), 1.0f, Color::Green);
	test_scene.objects.push_back(&test_sphere1);

	Sphere test_sphere2(Vector3f(0, 3, -5.0f), 0.8f, Color::Blue);
	test_scene.objects.push_back(&test_sphere2);

	Sphere test_sphere3(Vector3f(0.5f, 0.5f, -2.0f), 0.7f, Color::Red);
	test_scene.objects.push_back(&test_sphere3);

	Sphere test_sphere4(Vector3f(-0.4f, -0.4f, -1.0f), 0.5f, Color::White);
	test_scene.objects.push_back(&test_sphere4);

	Sphere test_sphere5(Vector3f(-2, -0, -8.0f), 2.0f, Color::Yellow);
	test_scene.objects.push_back(&test_sphere5);

	// A test Plane in the world
	//Plane* test_plane = new Plane(Vector3f(0, 0, 0), Vector3f(0, -1, 0) , Color::Red);
	//test_scene.objects.push_back(test_plane);

	// SFML Window creation
	sf::RenderWindow window(sf::VideoMode(image_width, image_height), "Raytracer v0.1");
	window.setActive(false);

	// Benchmarking of rendering call
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	render(test_scene, window);
	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	std::cout << "Rendered in " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "ms";

	
	// Main SFML loop
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
			{
				window.close();
				EXIT_SUCCESS;
			}
		}

		window.clear();
		cam.position = cam.position + Vector3f(0, 0, -0.02f);
		render(test_scene, window);
		window.draw(sprite);
		window.display();
	}

	return 0;
}

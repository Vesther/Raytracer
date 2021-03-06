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
#include <string>

#define M_PI 3.14159265358979323846
const int image_width = 1920;
const int image_height = 1080;

// Set this to true to enable animated rendering, eg. updating of the RenderWindow while it is rendering.
// Note: Very expensive, slows rendering by some 50%;
const bool pretty = false;

// If set to true, will render the result to result.png in the executable's directory in addition to displaying
const bool render_to_file = true;

class Light
{
public:
	Vector3f direction;
	Color color;
	float intensity;
	
	Light(Vector3f direction, Color color, float intensity) : direction(direction), color(color), intensity(intensity) {}
};

class SphericalLight
{
public:
	Vector3f position;
	Color color;
	float intensity;

	SphericalLight(Vector3f position, Color color, float intensity) : position(position), color(color), intensity(intensity) {}
};


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

	std::vector<Light*> lights;
	std::vector<SphericalLight*> spherical_lights;
	float shadow_bias;
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

class HitResult
{
public:
	Vector3f point;
	Vector3f surface_normal;
	Object* object;

	HitResult() : object(nullptr) {}
	HitResult(Vector3f point, Vector3f surface_normal, Object* object) : point(point), surface_normal(surface_normal), object(object) {}
};

HitResult trace(Ray &ray, const Scene &scene)
{
	float closest_distance = 0;
	Object *closest = nullptr;

	HitResult hit;

	for (unsigned int k = 0; k < scene.objects.size(); k++)
	{
		Object& obj = *scene.objects[k];
		// If we hit something, color it with the hit object's color.
		float distance = obj.intersects(ray);
		if (distance == -1) continue;
		if (distance < closest_distance || closest == nullptr)
		{
			closest_distance = distance;
			closest = &obj;
		}
	}

	if (closest != nullptr)
	{
		hit.object = closest;
		hit.point = ray.origin + (ray.direction * closest_distance);
		hit.surface_normal = hit.object->surface_normal(hit.point);
	}

	return hit;
}

Color get_pixel_color(Ray &ray, const Scene &scene)
{
	Color pixel_color(16, 16, 16);

	HitResult hit = trace(ray, scene);

	if (hit.object != nullptr)
	{

		pixel_color = Color(0, 0, 0);
		
		for (int i = 0; i < scene.lights.size(); i++)
		{
			Light& light = *scene.lights[i];
			Vector3f direction_to_light = light.direction * -1;
			// Check if it is in light
			Ray shadow_ray;
			shadow_ray.origin = hit.point + (hit.surface_normal * scene.shadow_bias);
			shadow_ray.direction = direction_to_light;
			HitResult shadow_trace = trace(shadow_ray, scene);
			bool in_light = shadow_trace.object == nullptr;
			float light_intensity = (in_light ? light.intensity : 0.0f);
			float light_power = std::max(hit.surface_normal.dot(direction_to_light) * light_intensity, 0.0f);
			Color light_color = light.color * light_power * hit.object->reflectivity;
			pixel_color = pixel_color + hit.object->color * light_color;
		}
		
		for (int i = 0; i < scene.spherical_lights.size(); i++)
		{
			SphericalLight& light = *scene.spherical_lights[i];
			// Get normalized vector defining the direction from our point to the light
			Vector3f direction_to_light = (light.position - hit.point).normalize();
			// Check if it is in light
			Ray shadow_ray;
			shadow_ray.origin = hit.point + (hit.surface_normal * scene.shadow_bias);
			shadow_ray.direction = direction_to_light;

			HitResult shadow_trace = trace(shadow_ray, scene);
			bool in_light = shadow_trace.object == nullptr; // In Light detection is fucked.
			//if (!in_light)
				//std::cout << direction_to_light;
			
			float light_intensity = (in_light ? light.intensity/(4*M_PI*std::powf((light.position - hit.point).length(),2)) : 0.0f);
			float light_power = std::abs(hit.surface_normal.dot(direction_to_light)) * light_intensity;
			Color light_color = light.color * light_power * hit.object->reflectivity;
			pixel_color = pixel_color + hit.object->color * light_color;
		}
	
	}

	return pixel_color;
}

void render_part(int line_from, int line_to, const Scene &scene, sf::RenderWindow* window)
{
	for (int i = line_from; i < line_to; i++)
	{
		for (int j = 0; j < scene.width; j++)
		{
			Ray ray = create_ray(j, i, scene);
			image.setPixel(j, i, get_pixel_color(ray, scene));
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
sf::Image render(Scene scene, sf::RenderWindow &window, int frameNo)
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

	if (render_to_file)
		image.saveToFile((std::string)"result" + std::to_string(frameNo) + (std::string)".png");

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
	test_scene.shadow_bias = 0.00001f;

	// A sphere to be rendered in the world
	Sphere test_sphere1(Vector3f(0, 0, -5.0f), 1.0f, Color::Green);
	test_scene.objects.push_back(&test_sphere1);

	Sphere test_sphere2(Vector3f(0, 3, -5.0f), 0.8f, Color::Blue);
	test_scene.objects.push_back(&test_sphere2);

	Sphere test_sphere3(Vector3f(1.0f, 0.5f, -2.0f), 0.7f, Color::Red);
	test_scene.objects.push_back(&test_sphere3);

	Sphere test_sphere4(Vector3f(-0.4f, -0.4f, -1.0f), 0.5f, Color::White);
	test_scene.objects.push_back(&test_sphere4);

	Sphere test_sphere5(Vector3f(-2, -0, -8.0f), 2.0f, Color::Yellow);
	test_scene.objects.push_back(&test_sphere5);

	Sphere test_sphere6(Vector3f(-2, 2, -3.5f), 1.0f, Color::Cyan);
	test_scene.objects.push_back(&test_sphere6);

	// A test Plane in the world
	Plane* test_plane = new Plane(Vector3f(0, 0, -10.0f), Vector3f(0, 0, -1), Color(135, 206, 255));
	test_plane->debugID = "BackPlane";
	test_scene.objects.push_back(test_plane);

	Plane* test_plane2 = new Plane(Vector3f(0, -2.0f, -10.0f), Vector3f(0, -1, 0), Color(64, 64, 64));
	test_plane2->debugID = "FloorPlane";
	//test_scene.objects.push_back(test_plane2);

	Light light1(Vector3f(1, -1, -1).normalize(), Color(255, 165, 0), 1.0f);
	test_scene.lights.push_back(&light1);

	Light light2(Vector3f(-1, -1, -1).normalize(), Color::Blue, 0.8f);
	test_scene.lights.push_back(&light2);

	Light light3(Vector3f(-1, -1, -2).normalize(), Color::Magenta, 0.3f);
	test_scene.lights.push_back(&light3);

	SphericalLight sLight1(Vector3f(0, 0, -2.0f), Color::White, 300.0f);
	test_scene.spherical_lights.push_back(&sLight1);

	// SFML Window creation
	sf::RenderWindow window(sf::VideoMode(image_width, image_height), "Raytracer v0.1");
	window.setActive(false);

	// Benchmarking of rendering call
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	render(test_scene, window, 0);
	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	std::cout << "Rendered in " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "ms" << "\n";

	int frameNo = 1;

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

			if (event.type == sf::Event::MouseButtonPressed)
			{
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					std::cout << "\nThe left button was pressed" << std::endl;
					std::cout << "--------------------------------------------" << std::endl;
					std::cout << "Clicked window x: " << event.mouseButton.x << std::endl;
					std::cout << "Clicked window y: " << event.mouseButton.y << std::endl;

					const int &x = event.mouseButton.x;
					const int &y = event.mouseButton.y;

					Ray ray = create_ray(x, y, test_scene);
					std::cout << "Ray direction: " << ray.direction << std::endl;
					std::cout << "Ray origin: " << ray.origin << std::endl;

					HitResult hit = trace(ray, test_scene);
					std::cout << "Hit object ID: " << hit.object->debugID << std::endl;
					std::cout << "Hit point: " << hit.point << std::endl;
					std::cout << "Hit surface normal: " << hit.surface_normal << std::endl;
					//hit.object->color = Color::Red;

					// Check for collision on the way to our sphereLight
					SphericalLight& light = *test_scene.spherical_lights[0];
					// Get normalized vector defining the direction from our point to the light
					Vector3f direction_to_light = (light.position - hit.point).normalize();
					Ray shadow_ray;
					shadow_ray.origin = hit.point + (hit.surface_normal * test_scene.shadow_bias);
					shadow_ray.direction = direction_to_light;

					HitResult shadow_trace = trace(shadow_ray, test_scene);
					bool in_light = shadow_trace.object == nullptr; // In Light detection is fucked.

					if(!in_light) 
						shadow_trace.object->color = Color(255, 105, 180);

					std::cout << "Point is in light: " << (in_light ? "TRUE" : "FALSE") << std::endl;
				}
			}
		}


		window.clear();
		sLight1.position = sLight1.position + Vector3f(0, 0.08f, 0.0f);
		cam.position = cam.position + Vector3f(0, 0, 0.01f);
		test_sphere1.center = test_sphere1.center + Vector3f(0, 0.1, 0.0f);
		test_sphere3.center = test_sphere3.center - Vector3f(-0.1, 0, 0.0f);

		render(test_scene, window, frameNo);
		window.draw(sprite);
		window.display();
		++frameNo;
	}

	return 0;
}

#include <iostream>
#include "Image.h"
#include "Sphere.h"
#include "Triangle.h"
#include "Camera.h"
#include "TGA.h"
#include "Raytracer.h"

static const int IMAGE_WIDTH = 500;
static const int IMAGE_HEIGHT = 500;

int main(int argc, char** argv)
{
    // Define scene
    Camera camera(
        Vector3(0, 300, 0), // position
        Vector3(0, 0, -1), // direction
        //Vector3(0.4f, 0.5f, -1), // direction
        Vector3(0, 1, 0), // up
        Rectangle(-400, 400, -400, 400), // viewing rectangle
        500
    );
    Raytracer raytracer(camera);
    raytracer.addShape(new Sphere(Vector3(250, 250, -1000), 150,
        Colour(0.2f, 0.2f, 0.8f), new Material(
            Colour(1.0f, 0.0f, 0.0f),
            Colour(0.0f, 1.0f, 0.0f),
            Colour(0.0f, 0.0f, 1.0f)
        )
    ));
    raytracer.addShape( new Triangle(
        Vector3(300, 600, -800),
        Vector3(0, 100, -1000),
        Vector3(450, 20, -1000),
        Colour(0.8f, 0.2f, 0.2f)
    ));
    raytracer.addShape( new Triangle(
        Vector3(0, 100, -250),
        Vector3(0, 200, -250),
        Vector3(100, 200, -250),
        Colour(0.8f, 0.7f, 0.2f)
    ));
    raytracer.addShape( new Triangle(
        Vector3(100, 0, -100),
        Vector3(100, 200, -100),
        Vector3(300, 200, -100),
        Colour(0.2f, 0.7f, 0)
    ));

    // Create object to store image output
    Colour backgroundColour(0.2f, 0.2f, 0.2f);
    Image output(IMAGE_WIDTH, IMAGE_HEIGHT);
    output.clear(backgroundColour);

    // Perform raytrace
    Colour resultantColour;
    // Loop over the pixels of the image
    for (int i = 0; (i < output.getWidth()); i++)
    {
        for (int j = 0; (j < output.getHeight()); j++)
        {
            // Convert pixel coordinates (i, k) to viewing plane coordinates (x, y)
            // Note that this gets the pixel CENTRE
            float x = (static_cast<float>(i) + 0.5f) / output.getWidth(); // a
            float y = (static_cast<float>(j) + 0.5f) / output.getHeight(); // n
            bool hit = raytracer.raytrace(x, y, resultantColour);
            if (hit)
                output.set(i, j, resultantColour);
            else
                output.set(i, j, backgroundColour);
        }
    }

    tga::writeTGAFile("output.tga", output);

    return 0;
}

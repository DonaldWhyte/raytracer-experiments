#include <iostream>
#include "Image.h"
#include "Sphere.h"
#include "Triangle.h"

int main(int argc, char** argv)
{
    // Define scene
    ShapeList shapes;
    shapes.push_back( new Sphere(
        Vector3(250, 250, -1000), 150, Colour(0.2f, 0.2f, 0.8f)
    ));
    shapes.push_back( new Triangle(
        Vector3(300, 600, -800),
        Vector3(0, 100, -1000),
        Vector3(450, 20, -1000),
        Colour(0.8f, 0.2f, 0.2f)
    ));

    // Create object to store image output
    Colour backgroundColour(0.2f, 0.2f, 0.2f);
    Image output(500, 500);
    output.clear(backgroundColour);

    // Perofrm raytrace
    HitRecord record;
    bool isAHit = false;
    float tMax;
    Vector3 direction(0, 0, -1); // direction through screen
    // Loop over the pixels of the image
    for (int i = 0; (i < output.getWidth()); i++)
    {
        for (int j = 0; (j < output.getHeight()); j++)
        {
            // Reset properties for ray trace
            tMax = 100000.0f;
            isAHit = false;
            // Construct ray to pass through pixel
            Ray ray(Vector3(i, j, 0), direction);

            for (int k = 0; (k < shapes.size()); k++)
            {
                if (shapes[k]->hit(ray, 0.00001f, tMax, 0.0f, record))
                {
                    tMax = record.t;
                    isAHit = true;
                }
            }
            // If an object was hit by the ray, then set the pixel's
            // colour to the colour of the object's surface
            if (isAHit)
            {
                output.set(i, j, record.colour);
            }
        }
    }

    output.toFile("output.tga");

    return 0;
}
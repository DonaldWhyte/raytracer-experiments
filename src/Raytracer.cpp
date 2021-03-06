#include "Raytracer.h"
#include "TerrainHeightTexture.h"
#include "Common.h"
#include <cmath>
#include <cfloat>
#include <algorithm>

using namespace raytracer;

Raytracer::Raytracer(const Camera& camera) :
    rootShape(NULL), rootTestShape(NULL), testShapesEnabled(false), camera(camera),
	localIllumEnabled(true), reflectRefractEnabled(true), shadowsEnabled(true)
{
    resetRayCount();
}

Raytracer::~Raytracer()
{
    delete rootShape;
    delete rootTestShape;
}

bool Raytracer::raytrace(float x, float y, Colour& result)
{
    // Construct Ray from Camera towards desired pixel
    Ray ray = camera.getRayToPixel(x, y);
    // Perform a recursive raytrace
    HitRecord record;
    bool isAHit = recursiveTrace(ray, record, 0);
    // If the ray hit an object, store resultant colour in OUT parameter
    if (isAHit)
        result = record.colour;

    numPrimaryRays++;

    return isAHit;
}

bool Raytracer::uniformMultisample(float minX, float minY, float maxX,
	float maxY, unsigned int samplesPerDirection, Colour& result)
{
	Colour sum;
	int hits = 0;
	// Based on desired number of samples, compute the size of the
	// steps taken by each interval
	float stepX = (maxX - minX) / samplesPerDirection;
	float stepY = (maxY - minY) / samplesPerDirection;
	for (unsigned int x = 0; (x < samplesPerDirection); x++)
	{
		for (unsigned int y = 0; (y < samplesPerDirection); y++)
		{
			// Determine sample based on current x and y coordinate on pixel's grid
			float sampleX = minX + (x * stepX);
			float sampleY = minY + (y * stepY);
		    Ray sampleRay = camera.getRayToPixel(sampleX, sampleY);
		    HitRecord record;
		    bool isHit = recursiveTrace(sampleRay, record, 0);
		    if (isHit)
		    {
		        sum += record.colour;
		        hits += 1;
		    }
        }
	}

    numPrimaryRays += (samplesPerDirection * samplesPerDirection);

	// Return average of all samples
    result = sum / std::max(1, hits);
    return (hits > 0);
}

bool Raytracer::randomMultisample(float minX, float minY, float maxX, float maxY,
    unsigned int samples, Colour& result)
{
    Colour sum;
    int hits = 0;
    for (unsigned int i = 0; (i < samples); i++)
    {
    	// Randomly generate point on viewing plane within range and cast ray to that pointshadowsEnabled
    	float sampleX = common::randomFloat(minX, maxX);
    	float sampleY = common::randomFloat(minY, maxY);
        Ray sampleRay = camera.getRayToPixel(sampleX, sampleY);
        HitRecord record;
        bool isHit = recursiveTrace(sampleRay, record, 0);
        if (isHit)
        {
            sum += record.colour;
            hits += 1;
        }
    }

    numPrimaryRays += samples;

	// Return average of all samples
    result = sum / std::max(1, hits);
    return (hits > 0);
}

void Raytracer::setRootShape(Shape* newRoot, bool deletePrevious)
{
    if (deletePrevious)
        delete rootShape; // delete the old root shape if specified!
    rootShape = newRoot;
}

Shape* Raytracer::getRootShape()
{
	return rootShape;
}

void Raytracer::addLight(const PointLight& light)
{
    lights.push_back(light);
}

void Raytracer::removeAllLights()
{
    lights.clear();
}

Camera* Raytracer::getCamera()
{
    return &camera;
}

bool Raytracer::showingTestShapes() const
{
    return testShapesEnabled;
}

void Raytracer::setRootTestShape(Shape* newRootTest, bool deletePrevious)
{
    if (deletePrevious)
        delete rootTestShape;
    rootTestShape = newRootTest;
}

void Raytracer::showTestShapes(bool show)
{
    testShapesEnabled = show;
}

/* Help from following sources:
 * http://www.baylee-online.net/Projects/Raytracing/Algorithms/Basic-Raytrace
 * http://www.cs.jhu.edu/~cohen/RendTech99/Lectures/Ray_Tracing.bw.pdf
 * https://github.com/jelmervdl/raytracer/blob/master/scene.cpp
*/
bool Raytracer::recursiveTrace(const Ray& ray, HitRecord& record, int depth)
{
    // Ensure recursive raytracer does not exceed maximum depth
    if (depth > MAX_TRACE_DEPTH) return false;
    if (!rootShape) return false;

    // If test shapes are enabled, be sure to test intersection with those as well
    float maxDistance = MAX_RAY_DISTANCE;
    bool testHit = false;
    if (testShapesEnabled)
    {
        testHit = rootTestShape->hit(ray, 0.0001f, maxDistance, 0.0f, record);
        if (testHit)
        {
            record.colour = TEST_SHAPE_COLOUR;
            maxDistance = record.t;
        }
    }
   
    bool objectHit = rootShape->hit(ray, 0.00001f, maxDistance, 0.0f, record);   
    if (objectHit)
    {
		// Get hit object's material and derive source object colour from it
		const Material* material = record.hitShape->getMaterial();
		Colour objectColour;
		if (material)
		    if (material->getTexture())
		    {
		    	Texture* texture = material->getTexture();
		    	// If the texture is a multitexture, use HEIGHT (Y)of point of intersection
		    	// to determine the weightings of each image.
		    	TerrainHeightTexture* terrainTexture = dynamic_cast<TerrainHeightTexture*>(texture);
		    	if (terrainTexture)
		    	{
			    	// height (y) is normalised by the maximum height of terrain
			    	// before using it to update terrain texture weights
			    	// NOTE: 0.75 coefficient used on max height to produce
			    	// weights which give better looking terrain
			    	float normalisedHeight = (record.pointOfIntersection.y / (common::TERRAIN_MAX_HEIGHT * 0.75));
		    		terrainTexture->updateWeights(normalisedHeight);
		    	}
		    	// Now get the texture's textel at the given texture coordinates
			    objectColour = texture->getTexel(record.texCoord.x, record.texCoord.y);
		    }
		    else
		    {
		        objectColour = material->getColour();
		    }
		else
		{
		    material = &defaultMaterial;
		}
        // Compute contributions of different physical phenoma to final colour
        Colour localColour, reflectedRefractedColour;
        if (localIllumEnabled)
	        localColour = localIllumination(material, objectColour, record);
	    else // if not enbled, just use object's colour directly
	    	localColour = objectColour;
	    if (reflectRefractEnabled)
        	reflectedRefractedColour = reflectionAndRefraction(ray.direction(), record, depth);
        // Combine computed colours into one
        record.colour = (LOCAL_ILLUMINATION_WEIGHT * localColour)
            + (REFLECTED_REFRACTED_WEIGHT * reflectedRefractedColour);
    }

    return (testHit || objectHit);
}

Colour Raytracer::localIllumination(const Material* material, const Colour& objectColour, const HitRecord& record)
{
    Colour localColour;
    // Add illumination to object for each light source in the scene
    for (int i = 0; (i < lights.size()); i++)
    {
        const Vector3& lightPos = lights[i].getPosition();
        Vector3 lightDirection = (lightPos - record.pointOfIntersection).normalise();
        // Ambient lighting
        localColour += (lights[i].getAmbient() * objectColour * material->ambientIntensity());

		if (shadowsEnabled)
		{
		    // Don't add diffuse and specular contribution from this light
		    // if the light is being blocked by another object
		    Ray lightRay( lightPos, (record.pointOfIntersection - lightPos).normalise() );
		    // Compute distance from light to point of intersection.
		    // This is used to ignore any shapes that are FURTHER AWAY
		    // FROM THE LIGHT SOURCE than the current object
		    float distanceFromLightToPoint = (record.pointOfIntersection - lightPos).length();

		    // Store shape which the ray hit!
		    const Shape* occludingShape = NULL;
		    bool shadowHit = rootShape->shadowHit(lightRay, 0.00001f,
		        distanceFromLightToPoint - SHADOW_RAY_DISTANCE_THRESHOLD,
		        0.0f, occludingShape);
		    numShadowRays++;
		    // If another object has blocked light reaching current object, don't add light contribution!
		    if (shadowHit)
		        if (record.hitShape != occludingShape)
		            continue;
		}

        // Diffuse lighting
        float angle = lightDirection.dot(record.normal);
        if (angle > 0.0f) // only diffuse light coming from FRONT will be considered
            localColour += lights[i].getDiffuse() * objectColour * material->diffuseIntensity() * angle;
        // Specular lighting
        // Use lightDir DOT normal to compute reflection direction
        Vector3 reflectionDirection = -(lightDirection - (2.0f * angle * record.normal));
        float reflectionAngle = reflectionDirection.dot(lightDirection);
        if (reflectionAngle > 0) // only specular light from FRONT will be considered
        {
            localColour += (lights[i].getSpecular() * material->specularIntensity()
                * pow(reflectionAngle, material->specularExponent()));
        }
    }

    return localColour;
}

Colour Raytracer::reflectionAndRefraction(const Vector3& rayDirection, const HitRecord& record, int depth)
{
    // Retrieve material properties
    const Material* material = record.hitShape->getMaterial();
    float reflectivity = material->reflectivity();
    float refractiveIndex = material->refractiveIndex();

    // If the hit object has no reflective nor refractive properties, just return no colour
    if (reflectivity == Material::NO_REFLECTION && refractiveIndex == Material::NO_REFRACTION)
        return Colour();

    // Retrieve refractive index of object ray orignated from
    // (refractive index of air is used as a default is the
    // ray was one which was originally cast through the viewing plane)
    float originRefractiveIndex = Material::AIR_REFRACTIVE_INDEX;
    if (record.originShape)
        originRefractiveIndex = record.originShape->getMaterial()->refractiveIndex();

    // Compute contribution of reflected and refracted colour
    // (contributions should sum up to one!)
    float reflectionFactor = reflectivity;
    float refractionFactor = 0.0f;
    if (refractiveIndex != Material::NO_REFRACTION)
    {
        // Compute surface's actual reflectivity BASED on its refraction index
        // (and the refraction index of the previous medium the ray was travelling through)
        reflectionFactor = computeSurfaceReflectivity(rayDirection,
            record.normal, originRefractiveIndex, refractiveIndex);
        // Transmittence is one minus reflection (Realistic Ray Reacing, page 177)
        refractionFactor = 1.0f - reflectionFactor;
    }
    // If there is no contribution from either, then return no colour
    if (reflectionFactor <= 0 && refractionFactor <= 0)
        return Colour();

    // Handle reflection if material of hit shape is reflective
    Colour reflectedColour;
    if (reflectionFactor > 0.0f)
    {
        Ray reflectedRay(record.pointOfIntersection, record.normal);
        // Cast reflected ray and store resultant colour
        HitRecord reflectRecord;
        if (recursiveTrace(reflectedRay, reflectRecord, depth + 1))
            reflectedColour = reflectRecord.colour;
        numReflectedRays++;
    }
    // Handle refraction if material of hit shape is refractive
    Colour refractedColour;
    if (refractionFactor > 0.0f)
    {
        // Compute refracted ray
        Ray refractedRay;
        bool shouldRefract = computeRefractedRay(rayDirection,
            record.pointOfIntersection, record.normal,
            originRefractiveIndex, refractiveIndex, refractedRay);
        if (shouldRefract) // false if total internal reflection occurred
        {
            // Cast refracted ray and store resultant colour
            HitRecord refractionRecord;
            if (recursiveTrace(refractedRay, refractionRecord, depth + 1))
                refractedColour = refractionRecord.colour;
        }
        numRefractedRays++;
    }

    return (reflectedColour * reflectionFactor) + (refractedColour * refractionFactor);
}

/* The following two methods were implemented with help from Realistic
 * Raytracing (pages 175-178) and:
 * http://steve.hollasch.net/cgindex/render/refraction.txt */
float Raytracer::computeSurfaceReflectivity(const Vector3& incoming,
    const Vector3& surfaceNormal, float originRefractiveIndex,
    float hitRefractiveIndex)
{
    // Compute angle of refraction using refractive indices
    double n = originRefractiveIndex / hitRefractiveIndex;
    double cosIncoming = -(surfaceNormal.dot(incoming)); // angle of incoming direction
    double sinT2 = n * n * (1.0 - cosIncoming * cosIncoming);
    // Check for total internal reflection (then we have full reflectance)
    // (because cosT = sqrt(0))
    if (sinT2 > 1.0)
        return 1.0;
    // Get cosine of refracted angle
    double cosT = sqrt(1.0f - sinT2);
    // Use this to compute the reflectivity of the surface with
    // regards to the refractive indices (Realistic Raytracing, Equation 12.7)
    double reflOrigin = (originRefractiveIndex * cosIncoming - hitRefractiveIndex * cosT) / (originRefractiveIndex * cosIncoming + hitRefractiveIndex * cosT);
    double reflHit = (hitRefractiveIndex * cosIncoming - originRefractiveIndex * cosT) / (hitRefractiveIndex * cosIncoming + originRefractiveIndex * cosT);
    return (reflOrigin * reflOrigin + reflHit * reflHit) / 2.0f;
}

bool Raytracer::computeRefractedRay(const Vector3 incomingDirection,
    const Vector3& pointOfIntersection, const Vector3& surfaceNormal,
    float refractiveIndex1, float refractiveIndex2, Ray& result)
{
    // NOTE: For simplicity, it is assumed that all rays were
    // travelling through the air BEFORE they hit the surface
    // that is refracting light

    // Equation: cos^2(theta) = 1 - ((n^2 * (1 - cos^2(theta))) / (n_t^2))
    float n = refractiveIndex1 / refractiveIndex2;
    float cosIncoming = -(incomingDirection.dot(surfaceNormal));
    float sinT2 = n * n * (1.0 - cosIncoming * cosIncoming); // from Snell's law
    if (sinT2 <= 1.0f)
    {
        float cosT = sqrt(1 - sinT2);
        Vector3 refractionDirection = (n * incomingDirection) + ((n * cosIncoming - cosT) * surfaceNormal);
        result = Ray(pointOfIntersection, refractionDirection);
        return true;
    }
    // If sinT2 > 1, then we have total internal reflection and NO REFRACTION.
    // To handle this, 'false' is returned to tell the calling code that
    // no refraction ray could be computed
    else
    {
        return false;
    }
}

void Raytracer::enableLocalIllumination(bool enabled)
{
	localIllumEnabled = enabled;
}

void Raytracer::enableReflectionAndRefraction(bool enabled)
{
	reflectRefractEnabled = enabled;
}

void Raytracer::enableShadows(bool enabled)
{
	shadowsEnabled = enabled;
}

unsigned int Raytracer::primaryRays() const
{
    return numPrimaryRays;
}

unsigned int Raytracer::reflectedRays() const
{
    return numReflectedRays;
}

unsigned int Raytracer::refractedRays() const
{
    return numRefractedRays;
}

unsigned int Raytracer::shadowRays() const
{
    return numShadowRays;
}

unsigned int Raytracer::totalRays() const
{
    return numPrimaryRays + numReflectedRays + numRefractedRays + numShadowRays;
}

void Raytracer::resetRayCount()
{
    numPrimaryRays = 0;
    numReflectedRays = 0;
    numRefractedRays = 0;
    numShadowRays = 0;
}

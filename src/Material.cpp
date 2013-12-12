#include "Material.h"

Material::Material() : ka(0), kd(0), ks(0), n(0),
    reflectionFactor(0), transparencyFactor(0), texture(NULL)
{
}

Material::Material(float ka, float kd, float ks, float n,
    float reflectionFactor, float transparencyFactor,
    const Colour& colour,Texture* texture) :
    ka(ka), kd(kd), ks(ks), n(n), reflectionFactor(reflectionFactor),
    transparencyFactor(transparencyFactor), colour(colour),
    texture(texture)
{
}

float Material::ambientIntensity() const
{
    return ka;
}

float Material::diffuseIntensity() const
{
    return kd;
}

float Material::specularIntensity() const
{
    return ks;
}

float Material::specularExponent() const
{
    return n;
}

float Material::reflectivity() const
{
    return reflectionFactor;
}

float Material::transparency() const
{
    return transparencyFactor;
}

const Colour& Material::getColour() const
{
    return colour;
}

Texture* Material::getTexture() const
{
    return texture;
}

void Material::setAmbientIntensity(float newKA)
{
    ka = newKA;
}

void Material::setDiffuseIntensity(float newKD)
{
    kd = newKD;
}

void Material::setSpecularIntensity(float newKS)
{
    ks = newKS;
}

void Material::setSpecularExponent(float newN)
{
    n = newN;
}

void Material::setReflectivity(float newReflectionFactor)
{
    reflectionFactor = newReflectionFactor;
}

void Material::setTransparency(float newTransparencyFactor)
{
    transparencyFactor = newTransparencyFactor;
}

void Material::setColour(const Colour& newColour)
{
    colour = newColour;
}

void Material::setTexture(Texture* newTexture)
{
    texture = newTexture;
}

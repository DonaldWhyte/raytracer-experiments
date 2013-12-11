#ifndef DW_RAYTRACER_MESH_TRIANGLE_H
#define DW_RAYTRACER_MESH_TRIANGLE_H

#include "Shape.h"
#include "Mesh.h"

class MeshTriangle : public Shape
{

public:
    MeshTriangle(Mesh* mesh, int v1, int v2, int v3);
    virtual ~MeshTriangle();

    // Overloading Material getter/setter so it uses the Mesh object's material
    virtual const Material* getMaterial() const;
    virtual void setMaterial(Material* newMaterial);

    bool hit(const Ray& ray, float tMin, float tMax, float time, HitRecord& record) const;
    bool shadowHit(const Ray& ray, float tMin, float tMax, float time) const;

private:
    Mesh* mesh;
    int v1, v2, v3;

};

#endif

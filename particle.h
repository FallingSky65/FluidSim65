#ifndef PARTICLE_H
#define PARTICLE_H

#include <raylib.h>
#include "params.h"

typedef struct Particle {
    Vector3 position, velocity, acceleration;
    float mass;

    float density;
    float pressure;
} Particle;

Particle particles[particle_count];

#endif // !PARTICLE_H
#define PARTICLE_H

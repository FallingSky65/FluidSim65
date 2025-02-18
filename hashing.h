#ifndef HASHING_H
#define HASHING_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "params.h"
#include "particle.h"

//https://web.archive.org/web/20071223173210/http://www.concentric.net/~Ttwang/tech/inthash.htm
int32_t hashInt32(int32_t key) {
    int32_t c2=0x27d4eb2d; // a prime or an odd constant
    key = (key ^ 61) ^ (key >> 16);
    key = key + (key << 3);
    key = key ^ (key >> 4);
    key = key * c2;
    key = key ^ (key >> 15);
    return key;
}

int32_t hash3D(int32_t x, int32_t y, int32_t z) {
    return (x * 18397) + (y * 20483) + (z * 29303);
    //int32_t prime = 53;
    //return ((prime + hashInt32(x)) * prime + hashInt32(y)) * prime + hashInt32(z);
}

int32_t get_table_index(size_t i) {
    int32_t xi = floor(particles[i].position.x/smoothing_radius);
    int32_t yi = floor(particles[i].position.y/smoothing_radius);
    int32_t zi = floor(particles[i].position.z/smoothing_radius);
    int32_t table_index = hash3D(xi, yi, zi) % table_size;
    if (table_index < 0) table_index += table_size;
    return table_index;
}

uint32_t hash_table[table_size + 1];
uint32_t dense_table[particle_count];

void spatial_hashing() {
    //memset(hash_table, 0, (table_size + 1)*sizeof(hash_table[0]));
    for (size_t i = 0; i <= table_size; i++) hash_table[i] = 0;
    for (size_t i = 0; i < particle_count; i++) dense_table[i] = 0;
    for (size_t i = 0; i < particle_count; i++) {
        int32_t table_index = get_table_index(i);
        hash_table[table_index]++;
    }
    for (size_t i = 1; i <= table_size; i++)
        hash_table[i] += hash_table[i-1];
    if (hash_table[table_size] != particle_count) printf("error with partial sums\n");
    for (size_t i = 0; i < particle_count; i++) {
        int32_t table_index = get_table_index(i);
        hash_table[table_index]--;
        dense_table[hash_table[table_index]] = i;
    }
}

#endif // !HASHING_H

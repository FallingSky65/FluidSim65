#ifndef PARAMS_H
#define PARAMS_H

const int particle_count = 10000;
const int table_size = particle_count;
const float box_size = 12.f;
const float damping = 1.0f;
const float epsilon = 0.001f;

const float smoothing_radius = 1.0f;

const float ball_radius = 1.2f;
const float gas_constant = 800.f;
const float total_mass = 100.f;
const float gravity = 4.0f*box_size;
const float target_density = total_mass * 40.f / (box_size * box_size * box_size * 8.f);
const float viscosity_factor = 10.f;
const float max_color_speed = 5.f;

const int screen_width = 1920;
const int screen_height = 1080;
const int target_fps = 30;
const float downscale_factor = 2.f;
const int updates_per_frame = 1;
const float simulation_speed_multiplier = 6.f;
float dt = 1.f/(target_fps*updates_per_frame);

const int thread_count = 100;
const int particles_per_thread = particle_count/thread_count;

#endif // !PARAMS_H
#define PARAMS_H

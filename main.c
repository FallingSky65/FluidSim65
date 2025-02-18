#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define MY_CAMERA_DO_NOT_USE_MOUSE
#include "my_camera.h"
#include "params.h"
#include "particle.h"
#include "hashing.h"
#include "threading.h"

#define CAP_FPS
#define BLUR_DEPTH
#define USE_NORMAL_SHADER

Matrix box_scale, box_translate, box_rotate, box_transform;

RenderTexture2D buffer1, buffer2;

bool simulate = true;

float smoothing_kernel_spiky(float dst) {
    if (dst >= smoothing_radius) return 0.f;

    float volume = (PI * powf(smoothing_radius, 5.f)) * 0.8f;
    return (dst - smoothing_radius)*(dst - smoothing_radius)/volume;
}

float smoothing_kernel_spiky_derivative(float dst) {
    if (dst >= smoothing_radius) return 0.f;

    float volume = (PI * powf(smoothing_radius, 5.f)) * 0.8f;
    return 2.f*(dst - smoothing_radius)/volume;
}

float sample_density(Vector3 v) {
    float density = 1.f;
    int32_t xi = floor(v.x/smoothing_radius);
    int32_t yi = floor(v.y/smoothing_radius);
    int32_t zi = floor(v.z/smoothing_radius);

    for (int dx = -1; dx <= 1; dx++) for (int dy = -1; dy <= 1; dy++) for (int dz = -1; dz <= 1; dz++) {
        int32_t table_index = hash3D(xi+dx, yi+dy, zi+dz) % table_size;
        if (table_index < 0) table_index += table_size;
        for (size_t j = hash_table[table_index]; j < hash_table[table_index+1]; j++) {
            Particle p = particles[dense_table[j]];
            if (dense_table[j] < 0 || dense_table[j] >= particle_count) printf("skib\n");
            density += p.mass * smoothing_kernel_spiky(Vector3Distance(v, p.position));
            //density += 1;
        }
    }

    return density;
}

Vector3 sample_pressure_force(Vector3 v, float pressure) {
    Vector3 pressure_force = Vector3Zero();
    int32_t xi = floor(v.x/smoothing_radius);
    int32_t yi = floor(v.y/smoothing_radius);
    int32_t zi = floor(v.z/smoothing_radius);

    for (int dx = -1; dx <= 1; dx++) for (int dy = -1; dy <= 1; dy++) for (int dz = -1; dz <= 1; dz++) {
        int32_t table_index = hash3D(xi+dx, yi+dy, zi+dz) % table_size;
        if (table_index < 0) table_index += table_size;
        for (size_t j = hash_table[table_index]; j < hash_table[table_index+1]; j++) {
            Particle p = particles[dense_table[j]];
            Vector3 offset = Vector3Subtract(p.position, v);
            float dst = Vector3Length(offset);
            if (dst < epsilon) continue;
            Vector3 dir = Vector3Scale(offset, 1.f/dst);
            float slope = smoothing_kernel_spiky_derivative(dst);
            pressure_force = Vector3Add(pressure_force, Vector3Scale(dir, slope*(p.pressure + pressure)*0.5f*p.mass/p.density));
        }
    }

    return pressure_force;
}

float smoothing_kernel(float dst) {
    if (dst >= smoothing_radius) return 0.f;

    float volume = (PI * powf(smoothing_radius, 13.f)) * 4096.f/3003.f;
    return powf(smoothing_radius*smoothing_radius - dst*dst, 3.f)/volume;
}

float smoothing_kernel_derivative(float dst) {
    if (dst >= smoothing_radius) return 0.f;

    float volume = (PI * powf(smoothing_radius, 13.f)) * 4096.f/3003.f;
    return -6.f*dst*powf(smoothing_radius*smoothing_radius - dst*dst, 2.f)/volume;
}

float smoothing_kernel_laplacian(float dst) {
    if (dst >= smoothing_radius) return 0.f;

    float volume = (PI * powf(smoothing_radius, 13.f)) * 4096.f/3003.f;
    return 24.f*dst*(smoothing_radius*smoothing_radius - dst*dst)/volume;
}

Vector3 sample_viscosity_force(Vector3 v, Vector3 velocity) {
    Vector3 viscosity_force = Vector3Zero();
    int32_t xi = floor(v.x/smoothing_radius);
    int32_t yi = floor(v.y/smoothing_radius);
    int32_t zi = floor(v.z/smoothing_radius);

    for (int dx = -1; dx <= 1; dx++) for (int dy = -1; dy <= 1; dy++) for (int dz = -1; dz <= 1; dz++) {
        int32_t table_index = hash3D(xi+dx, yi+dy, zi+dz) % table_size;
        if (table_index < 0) table_index += table_size;
        for (size_t j = hash_table[table_index]; j < hash_table[table_index+1]; j++) {
            Particle p = particles[dense_table[j]];
            float dst = Vector3Distance(p.position, v);
            if (dst < epsilon) continue;
            viscosity_force = Vector3Add(viscosity_force, Vector3Scale(Vector3Subtract(p.velocity, velocity), smoothing_kernel_laplacian(dst)*viscosity_factor*p.mass/p.density));
        }
    }

    return viscosity_force;
}

void set_position(size_t i) {
    particles[i].position = Vector3Add(particles[i].position, Vector3Scale(particles[i].velocity, dt*simulation_speed_multiplier));
}

void set_density_pressure(size_t i) {
    particles[i].density = sample_density(particles[i].position);
    particles[i].pressure = gas_constant * (particles[i].density - target_density);
}

void set_acceleration(size_t i) {
    Vector3 net_force = { 0.f, -1.f*gravity*particles[i].mass, 0.f };
    net_force = Vector3Add(net_force, sample_pressure_force(particles[i].position, particles[i].pressure));
    net_force = Vector3Add(net_force, sample_viscosity_force(particles[i].position, particles[i].velocity));

    particles[i].acceleration = Vector3Scale(net_force, 1.f/particles[i].density);
}

void set_velocity_position(size_t i) {
    particles[i].position = Vector3Subtract(particles[i].position, Vector3Scale(particles[i].velocity, dt*simulation_speed_multiplier));
    particles[i].velocity = Vector3Add(particles[i].velocity, Vector3Scale(particles[i].acceleration, dt*simulation_speed_multiplier));
    particles[i].position = Vector3Add(particles[i].position, Vector3Scale(particles[i].velocity, dt*simulation_speed_multiplier));
}

Vector4 Vector4Transform(Vector4 v, Matrix mat)
{
    Vector4 result = { 0 };

    float x = v.x;
    float y = v.y;
    float z = v.z;
    float w = v.w;

    result.x = mat.m0*x + mat.m4*y + mat.m8*z + mat.m12*w;
    result.y = mat.m1*x + mat.m5*y + mat.m9*z + mat.m13*w;
    result.z = mat.m2*x + mat.m6*y + mat.m10*z + mat.m14*w;
    result.w = mat.m3*x + mat.m7*y + mat.m11*z + mat.m15*w;

    return result;
}

void collide_walls(size_t i) {
    Matrix inverted = MatrixInvert(box_transform);
    particles[i].position = Vector3Transform(particles[i].position, inverted);
    Vector4 vel = {
        .x = particles[i].velocity.x,
        .y = particles[i].velocity.y,
        .z = particles[i].velocity.z,
        .w = 0.f
    };
    vel = Vector4Transform(vel, inverted);

    // ball wall
        if (particles[i].position.x < -1.f) {
            particles[i].position.x = -1.f;
            if (vel.x < 0) vel.x *= -damping;
        }
        if (particles[i].position.y < -1.f) {
            particles[i].position.y = -1.f;
            if (vel.y < 0) vel.y *= -damping;
        }
        if (particles[i].position.z < -1.f) {
            particles[i].position.z = -1.f;
            if (vel.z < 0) vel.z *= -damping;
        }
        if (particles[i].position.x > 1.f) {
            particles[i].position.x = 1.f;
            if (vel.x > 0) vel.x *= -damping;
        }
        if (particles[i].position.y > 1.f) {
            particles[i].position.y = 1.f;
            if (vel.y > 0) vel.y *= -damping;
        }
        if (particles[i].position.z > 1.f) {
            particles[i].position.z = 1.f;
            if (vel.z > 0) vel.z *= -damping;
        }

    particles[i].position = Vector3Transform(particles[i].position, box_transform);
    vel = Vector4Transform(vel, box_transform);
    particles[i].velocity = (Vector3){ vel.x, vel.y, vel.z };
}

void update_particles() {
    thread_master(set_position);
    thread_master(set_density_pressure);

    thread_master(set_acceleration);
    thread_master(set_velocity_position);

    thread_master(collide_walls);
}

Color gradient(float t) {
    t /= max_color_speed;
    t = Clamp(1.f - t, 0.f, 1.f);
    return ColorFromHSV(240.f*t, 1.f, 1.f);
}

void update_transforms(Matrix *transforms) {
    for (size_t i = 0; i < particle_count; i++) {
        /*
        transforms[i] = MatrixTranslate(
            particles[i].position.x,
            particles[i].position.y,
            particles[i].position.z
        );
        */
        
        transforms[i] = MatrixIdentity();

        transforms[i].m0 = particles[i].position.x;
        transforms[i].m1 = particles[i].position.y;
        transforms[i].m2 = particles[i].position.z;

        transforms[i].m4 = my_camera.position.x;
        transforms[i].m5 = my_camera.position.y;
        transforms[i].m6 = my_camera.position.z;

        Color color = gradient(Vector3Length(particles[i].velocity));

        transforms[i].m8 = color.r/255.f;
        transforms[i].m9 = color.g/255.f;
        transforms[i].m10 = color.b/255.f;
    }
}

void draw_box() {
    Vector3 p1 = Vector3Transform((Vector3){-1.f, -1.f, -1.f}, box_transform);
    Vector3 p2 = Vector3Transform((Vector3){-1.f, -1.f,  1.f}, box_transform);
    Vector3 p3 = Vector3Transform((Vector3){-1.f,  1.f, -1.f}, box_transform);
    Vector3 p4 = Vector3Transform((Vector3){-1.f,  1.f,  1.f}, box_transform);
    Vector3 p5 = Vector3Transform((Vector3){ 1.f, -1.f, -1.f}, box_transform);
    Vector3 p6 = Vector3Transform((Vector3){ 1.f, -1.f,  1.f}, box_transform);
    Vector3 p7 = Vector3Transform((Vector3){ 1.f,  1.f, -1.f}, box_transform);
    Vector3 p8 = Vector3Transform((Vector3){ 1.f,  1.f,  1.f}, box_transform);

    DrawLine3D(p1, p2, WHITE);
    DrawLine3D(p1, p3, WHITE);
    DrawLine3D(p1, p5, WHITE);
    DrawLine3D(p2, p4, WHITE);
    DrawLine3D(p2, p6, WHITE);
    DrawLine3D(p3, p4, WHITE);
    DrawLine3D(p3, p7, WHITE);
    DrawLine3D(p4, p8, WHITE);
    DrawLine3D(p5, p6, WHITE);
    DrawLine3D(p5, p7, WHITE);
    DrawLine3D(p6, p8, WHITE);
    DrawLine3D(p7, p8, WHITE);
}

RenderTexture2D LoadRenderTextureHQ(int width, int height)
{
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create color texture (default to RGBA)
        target.texture.id = rlLoadTexture(NULL, width, height, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;
        target.texture.mipmaps = 1;
        SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

        // Create depth renderbuffer/texture
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach color texture and depth renderbuffer/texture to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

void swap_buffers() {
    RenderTexture2D temp = buffer1;
    buffer1 = buffer2;
    buffer2 = temp;
}

int main() {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(screen_width, screen_height, "Fluid Sim 65");
    ToggleFullscreen();

    my_camera.position = (Vector3){ 0.f, 2.f*box_size, -3.6f*box_size };
    my_camera.yaw = 0.f;
    my_camera.pitch = PI*0.f;
    my_camera.move_speed = box_size;
    my_camera.rotation_speed = 0.08f;

    Camera camera = {
        .position = my_camera.position,
        .target = { 0.f, 0.f, 0.f },
        .up = AXIS_Y,
        .fovy = 60.f,
        .projection = CAMERA_PERSPECTIVE
    };

    Mesh particle_mesh = GenMeshPlane(ball_radius*2.f, ball_radius*2.f, 1, 1);//GenMeshSphere(ball_radius, 6, 12);

    box_scale = MatrixScale(box_size, box_size, box_size);
    box_rotate = MatrixIdentity();
    box_translate = MatrixTranslate(0.f, 2.f*box_size, 0.f);
    box_transform = MatrixMultiply(MatrixMultiply(box_scale, box_rotate), box_translate);

    for (size_t i = 0; i < particle_count; i++) {
        particles[i].position.x = GetRandomValue(-1000, 1000)*0.001f;
        particles[i].position.y = GetRandomValue(-1000,    0)*0.001f;
        particles[i].position.z = GetRandomValue(-1000, 1000)*0.001f;
        particles[i].position = Vector3Transform(particles[i].position, box_transform);

        particles[i].velocity = Vector3Zero();
        particles[i].acceleration = Vector3Zero();
        
        particles[i].mass = total_mass/particle_count;
    }

    Matrix *transforms = (Matrix*)RL_CALLOC(particle_count, sizeof(Matrix));

    buffer1 = LoadRenderTextureHQ(screen_width/downscale_factor, screen_height/downscale_factor);
    buffer2 = LoadRenderTextureHQ(screen_width/downscale_factor, screen_height/downscale_factor);

    Shader depth_shader = LoadShader(
        "depth_vert.glsl",
        "depth_frag.glsl"
    );

    // Get shader locations
    depth_shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(depth_shader, "mvp");
    depth_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(depth_shader, "viewPos");
    depth_shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(depth_shader, "instanceTransform");

    int ball_radius_loc = GetShaderLocation(depth_shader, "ball_radius");
    SetShaderValue(depth_shader, ball_radius_loc, &ball_radius, SHADER_UNIFORM_FLOAT);
    
    int view_matrix_pos = GetShaderLocation(depth_shader, "viewMatrix");
    int proj_matrix_pos = GetShaderLocation(depth_shader, "projMatrix");

    Shader blur_shader = LoadShader(0, "blur_naive_frag.glsl");

    int blur_screenSizeLoc = GetShaderLocation(blur_shader, "screenSize");
    int blur_axisLoc = GetShaderLocation(blur_shader, "axis");

    Vector2 screenSize = { .x = screen_width/downscale_factor, .y = screen_height/downscale_factor };
    SetShaderValue(blur_shader, blur_screenSizeLoc, &screenSize, SHADER_UNIFORM_VEC2);
    
    Shader normal_shader = LoadShader(0, "normal_frag.glsl");

    int normal_screenSizeLoc = GetShaderLocation(normal_shader, "screenSize");
    int normal_invProjLoc = GetShaderLocation(normal_shader, "invProjection");
    int normal_invViewLoc = GetShaderLocation(normal_shader, "invView");
    normal_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(normal_shader, "viewPos");

    SetShaderValue(normal_shader, normal_screenSizeLoc, &screenSize, SHADER_UNIFORM_VEC2);
    
    Material particle_mat_instanced = LoadMaterialDefault();
    particle_mat_instanced.shader = depth_shader;
    particle_mat_instanced.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

#ifndef MY_CAMERA_DO_NOT_USE_MOUSE
    DisableCursor();
    SetMousePosition(0, 0);
#endif
#ifdef CAP_FPS
    SetTargetFPS(target_fps);
#endif
    while (!WindowShouldClose())
    {  
        if (IsKeyReleased(KEY_P)) {
            simulate = !simulate;
        }
        if (IsKeyReleased(KEY_X)) {
            TakeScreenshot("screenshot.png");
        }

        update_camera(&camera, dt*updates_per_frame);
#ifndef MY_CAMERA_DO_NOT_USE_MOUSE
        SetMousePosition(0, 0);
#endif

        if (simulate) {
            box_rotate = MatrixMultiply(MatrixRotateZ(0.1f*dt*simulation_speed_multiplier), box_rotate);
            box_transform = MatrixMultiply(MatrixMultiply(box_scale, box_rotate), box_translate);

            for (size_t i = 0; i < updates_per_frame; i++) {
                spatial_hashing();
                update_particles();
            }
        }
        update_transforms(transforms);

        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(depth_shader, depth_shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(normal_shader, normal_shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        Matrix viewMatrix = GetCameraMatrix(camera);
        Matrix projMatrix = MatrixPerspective(camera.fovy*DEG2RAD, (double)screen_width/screen_height, rlGetCullDistanceNear(), rlGetCullDistanceFar());
        SetShaderValueMatrix(depth_shader, view_matrix_pos, viewMatrix);
        SetShaderValueMatrix(depth_shader, proj_matrix_pos, projMatrix);

        Matrix invView = MatrixInvert(viewMatrix);
        Matrix invProj = MatrixInvert(projMatrix);

        SetShaderValueMatrix(normal_shader, normal_invViewLoc, invView);
        SetShaderValueMatrix(normal_shader, normal_invProjLoc, invProj);

        BeginTextureMode(buffer1);
        {
            ClearBackground(BLACK);
            BeginMode3D(camera);
            {
                DrawMeshInstanced(particle_mesh, particle_mat_instanced, transforms, particle_count);
            }
            EndMode3D();
        }
        EndTextureMode();

#ifdef BLUR_DEPTH
        for (int axis = 0; axis <= 0; axis++) {
            swap_buffers();
            SetShaderValue(blur_shader, blur_axisLoc, &axis, SHADER_UNIFORM_INT);
            BeginTextureMode(buffer1);
            {
                BeginShaderMode(blur_shader);
                    DrawTextureRec(buffer2.texture, (Rectangle){.x = 0, .y = 0, .width = screen_width/downscale_factor, .height = -screen_height/downscale_factor}, Vector2Zero(), WHITE);
                EndShaderMode();
            }
            EndTextureMode();
        }
#endif

#ifdef USE_NORMAL_SHADER
        swap_buffers();
        BeginTextureMode(buffer1);
        ClearBackground((Color){ 0, 0, 0, 0 });
        BeginShaderMode(normal_shader);
            DrawTextureRec(buffer2.texture, (Rectangle){.x = 0, .y = 0, .width = screen_width/downscale_factor, .height = -screen_height/downscale_factor}, Vector2Zero(), WHITE);
        EndShaderMode();
        EndTextureMode();
#endif

        BeginDrawing();
        {
            ClearBackground(BLACK);
            //DrawTextureRec(buffer1.texture, (Rectangle){.x = 0, .y = 0, .width = screen_width, .height = -screen_height}, Vector2Zero(), WHITE);

            BeginMode3D(camera);
                draw_box();
                DrawGrid(2*(int)box_size, 2.f);
            EndMode3D();

            DrawTexturePro(buffer1.texture, (Rectangle){0, 0, screen_width/downscale_factor, -screen_height/downscale_factor}, (Rectangle){0, 0, screen_width, screen_height}, Vector2Zero(), 0.f, WHITE);

            DrawText(TextFormat("FluidSim65", particle_count), 10, 10, 20, WHITE);
            DrawText(TextFormat("%d particles", particle_count), 10, 40, 20, WHITE);
            DrawText(TextFormat("%d threads", thread_count), 10, 70, 20, WHITE);
            
            DrawFPS(10, screen_height - 60);
            DrawText(TextFormat("%d x %d", screen_width, screen_height), 10, screen_height - 30, 20, WHITE);
        }
        EndDrawing();
    }

    RL_FREE(transforms);

    CloseWindow();

    return 0;
}

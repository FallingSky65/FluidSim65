#ifndef MY_CAMERA_H
#define MY_CAMERA_H

#include "raylib.h"
#include "raymath.h"

struct {
    Vector3 position;
    float yaw, pitch; // ignore roll
    float move_speed;
    float rotation_speed;
} my_camera;

Vector3 AXIS_X = { 1.f, 0.f, 0.f };
Vector3 AXIS_Y = { 0.f, 1.f, 0.f };
Vector3 AXIS_Z = { 0.f, 0.f, 1.f };

void update_camera(Camera *camera, float dt) {
#ifndef MY_CAMERA_DO_NOT_USE_MOUSE
    Vector2 mouse_delta = GetMousePosition();
    my_camera.yaw -= mouse_delta.x * my_camera.rotation_speed * dt;
    my_camera.pitch += mouse_delta.y * my_camera.rotation_speed * dt;
    if (my_camera.pitch < -PI*0.499f) my_camera.pitch = -PI*0.499f;
    if (my_camera.pitch > PI*0.499f) my_camera.pitch = PI*0.499f;
#endif // !
    
    Vector3 forward = Vector3RotateByAxisAngle(Vector3RotateByAxisAngle(AXIS_Z, AXIS_X, my_camera.pitch), AXIS_Y, my_camera.yaw);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, AXIS_Y));
    Vector3 forward_move = Vector3Scale(Vector3RotateByAxisAngle(AXIS_Z, AXIS_Y, my_camera.yaw), my_camera.move_speed * dt);
    Vector3 right_move = Vector3Scale(right, my_camera.move_speed * dt);
    Vector3 up_move = Vector3Scale(camera->up, my_camera.move_speed * dt);
    
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        my_camera.position = Vector3Add(my_camera.position, forward_move);
    }
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
        my_camera.position = Vector3Subtract(my_camera.position, forward_move);
    }
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        my_camera.position = Vector3Subtract(my_camera.position, right_move);
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        my_camera.position = Vector3Add(my_camera.position, right_move);
    }
    if (IsKeyDown(KEY_SPACE)) {
        my_camera.position = Vector3Add(my_camera.position, up_move);
    }
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        my_camera.position = Vector3Subtract(my_camera.position, up_move);
    }

    camera->position = my_camera.position;
    camera->target = Vector3Add(camera->position, forward);
}

#endif // !MY_CAMERA_H
#define MY_CAMERA_H

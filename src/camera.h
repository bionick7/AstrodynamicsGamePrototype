#ifndef CAMERA_H
#define CAMERA_H

#include "basic.h"

typedef struct DrawCamera {
    float space_scale;
    float time_scale;
    bool paused;
} DrawCamera;

void MakeCamera(DrawCamera* cam);
Vector2 CameraTransformV(const DrawCamera* cam, Vector2 p);
Vector2 CameraInvTransformV(const DrawCamera* cam, Vector2 p);
double CameraTransformS(const DrawCamera* cam, double p);
void CameraTransformBuffer(const DrawCamera* cam, Vector2* buffer, int buffer_size);
time_type CameraAdvanceTime(const DrawCamera* cam, time_type t0, double delta_t);
void CameraHandleInput(DrawCamera* cam, double delta_t);
void CameraDrawUI(const DrawCamera* cam);
Vector2 GetMousePositionInWorld(const DrawCamera* cam);

#endif //CAMERA_H
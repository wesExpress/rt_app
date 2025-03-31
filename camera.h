#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "dm.h"

typedef struct simple_camera_t
{
    dm_vec3 pos, forward, up;
    dm_mat4 view, proj, vp;
} simple_camera;

void camera_init(simple_camera* camera, dm_context* context);
void camera_update(simple_camera* camera, dm_context* context);

#endif

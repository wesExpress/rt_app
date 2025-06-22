#include "camera.h"

void camera_init(float* pos, float* forward, simple_camera* camera, dm_context* context)
{
    dm_memcpy(camera->pos, pos, sizeof(dm_vec3));
    dm_vec3_norm(forward, camera->forward);
    camera->up[1]  = 1.f;
    
    dm_mat_view(camera->pos, camera->forward, camera->up, camera->view);
    dm_mat_perspective(DM_MATH_DEG_TO_RAD * 75.f, (float)context->renderer.width / (float)context->renderer.height, 0.1f, 1000.f, camera->proj);

    dm_mat4_mul_mat4(camera->view, camera->proj, camera->vp);

    dm_mat4_inverse(camera->view, camera->inv_view);
    dm_mat4_inverse(camera->proj, camera->inv_proj);
    dm_mat4_inverse(camera->vp,   camera->inv_vp);
}

void camera_update(simple_camera* camera, dm_context* context)
{
    dm_vec3 move = { 0 };

    // z direction (into screen is negative)
    if(dm_input_is_key_pressed(DM_KEY_W, context))
    {
        move[2] = -1.f;
    }
    else if(dm_input_is_key_pressed(DM_KEY_S, context))
    {
        move[2] = 1.f;
    }

    // x direction
    if(dm_input_is_key_pressed(DM_KEY_A, context))
    {
        move[0] = -1.f;
    }
    else if(dm_input_is_key_pressed(DM_KEY_D, context))
    {
        move[0] = 1.f;
    }

#define MOVE_SPEED 5.f
    dm_vec3_norm(move, move);
    dm_vec3_scale(move, MOVE_SPEED * context->delta, move);
    if(dm_vec3_mag2(move)>=0) dm_vec3_add_vec3(camera->pos, move, camera->pos);

    dm_vec3 target = { 0 };
    dm_vec3_add_vec3(camera->pos, camera->forward, target);
    dm_mat_view(camera->pos, target, camera->up, camera->view);

    dm_mat4_mul_mat4(camera->view, camera->proj, camera->vp);

    dm_mat4_inverse(camera->view, camera->inv_view);
    dm_mat4_inverse(camera->proj, camera->inv_proj);
    dm_mat4_inverse(camera->vp,   camera->inv_vp);
}

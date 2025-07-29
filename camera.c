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
        move[2] = 1.f;
    }
    else if(dm_input_is_key_pressed(DM_KEY_S, context))
    {
        move[2] = -1.f;
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

    if(dm_input_is_key_pressed(DM_KEY_E, context))
    {
        move[1] = 1.f;
    }
    else if(dm_input_is_key_pressed(DM_KEY_Q, context))
    {
        move[1] = -1.f;
    }

    dm_vec3 right;
    dm_vec3_cross(camera->forward, camera->up, right);

    if(dm_input_is_key_pressed(DM_KEY_LCTRL, context) && dm_input_mouse_has_moved(context))
    {
        int x, y;
        dm_input_get_mouse_delta(&x,&y, context);

        float delta_x = (float)x * 100.f * context->delta;
        float delta_y = (float)y * 100.f * context->delta;

        dm_quat q1, q2, r;


        dm_quat_from_axis_angle_deg(right, -delta_y, q1);
        dm_quat_from_axis_angle_deg(camera->up, -delta_x, q2);

        dm_quat_cross(q1,q2, r);
        dm_quat_norm(r,r);

        dm_vec3_rotate(camera->forward, r, camera->forward);

        dm_vec3_norm(camera->forward, camera->forward);

        dm_vec3_rotate(move, r, move);
    }

#define MOVE_SPEED 5.f
    dm_vec3 temp, move_vec;
    dm_vec3_scale(right, move[0], move_vec);
    dm_vec3_scale(camera->forward, move[2], temp);
    dm_vec3_add_vec3(move_vec, temp, move_vec);
    dm_vec3_scale(camera->up, move[1], temp);
    dm_vec3_add_vec3(move_vec, temp, move_vec);

    dm_vec3_norm(move_vec, move_vec);
    dm_vec3_scale(move_vec, MOVE_SPEED * context->delta, move_vec);
    if(dm_vec3_mag2(move_vec)>=0) dm_vec3_add_vec3(camera->pos, move_vec, camera->pos);

    dm_vec3 target = { 0 };
    dm_vec3_add_vec3(camera->pos, camera->forward, target);
    dm_mat_view(camera->pos, target, camera->up, camera->view);

    dm_mat4_mul_mat4(camera->view, camera->proj, camera->vp);

    dm_mat4_inverse(camera->view, camera->inv_view);
    dm_mat4_inverse(camera->proj, camera->inv_proj);
    dm_mat4_inverse(camera->vp,   camera->inv_vp);
}

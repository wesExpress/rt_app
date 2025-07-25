#ifndef __DATA_H__
#define __DATA_H__

#include "dm.h"

#if 0
#define INDEX_TYPE DM_INDEX_BUFFER_INDEX_TYPE_UINT16
typedef uint16_t index_t;
#else
#define INDEX_TYPE DM_INDEX_BUFFER_INDEX_TYPE_UINT32
typedef uint32_t index_t;
#endif

typedef struct vertex_t
{
    dm_vec4 pos;
    dm_vec4 normal;
    dm_vec4 color;
} vertex;

static const vertex triangle[] = {
    { {  0.f, -0.5f,0.f }, { 0,0,1 }, { 0.f,1.f,0.f,1.f } },
    { { -0.5f, 0.5f,0.f }, { 0,0,1 }, { 1.f,0.f,0.f,1.f } },
    { {  0.5f, 0.5f,0.f }, { 0,0,1 }, { 0.f,0.f,1.f,1.f } }
};

static const index_t triangle_indices[] = { 0,1,2 };

static const vertex quad[] = {
    { { -0.5f,-0.5f,0.f }, { 0,0,1 }, { 1.f,0.f,0.f,1.f } },
    { {  0.5f,-0.5f,0.f }, { 0,0,1 }, { 0.f,1.f,0.f,1.f } },
    { { -0.5f, 0.5f,0.f }, { 0,0,1 }, { 0.f,0.f,1.f,1.f } },
    { {  0.5f, 0.5f,0.f }, { 0,0,1 }, { 1.f,1.f,0.f,1.f } }
};

static const index_t quad_indices[] = { 0,1,2, 3,2,1 };

static const vertex cube[] = {
    // front face
    { { -0.5f,-0.5f,0.5f }, { 0,0,1 }, { 1.0f,0.f,0.f,1.f } },  // 0
    { {  0.5f,-0.5f,0.5f }, { 0,0,1 }, { 1.0f,0.f,0.f,1.f } },  // 1
    { { -0.5f, 0.5f,0.5f }, { 0,0,1 }, { 1.0f,0.f,0.f,1.f } },  // 2
    { {  0.5f, 0.5f,0.5f }, { 0,0,1 }, { 1.0f,0.f,0.f,1.f } },  // 3

    // back face
    { { -0.5f,-0.5f,-0.5f }, { 0,0,-1 }, { 0.0f,1.f,0.f,1.f } }, // 4
    { {  0.5f,-0.5f,-0.5f }, { 0,0,-1 }, { 0.0f,1.f,0.f,1.f } }, // 5
    { { -0.5f, 0.5f,-0.5f }, { 0,0,-1 }, { 0.0f,1.f,0.f,1.f } }, // 6
    { {  0.5f, 0.5f,-0.5f }, { 0,0,-1 }, { 0.0f,1.f,0.f,1.f } }, // 7
    
    // left face
    { { -0.5f,-0.5f,-0.5f }, { -1,0,0 }, { 0.0f,0.f,1.f,1.f } },  // 8
    { { -0.5f,-0.5f, 0.5f }, { -1,0,0 }, { 0.0f,0.f,1.f,1.f } },  // 9
    { { -0.5f, 0.5f, 0.5f }, { -1,0,0 }, { 0.0f,0.f,1.f,1.f } },  // 10
    { { -0.5f, 0.5f,-0.5f }, { -1,0,0 }, { 0.0f,0.f,1.f,1.f } },  // 11

    // right face
    { {  0.5f,-0.5f,-0.5f }, { 1,0,0 }, { 1.0f,1.f,0.f,1.f } },  // 12 
    { {  0.5f,-0.5f, 0.5f }, { 1,0,0 }, { 1.0f,1.f,0.f,1.f } },  // 13
    { {  0.5f, 0.5f, 0.5f }, { 1,0,0 }, { 1.0f,1.f,0.f,1.f } },  // 14
    { {  0.5f, 0.5f,-0.5f }, { 1,0,0 }, { 1.0f,1.f,0.f,1.f } },  // 15
    
    // top face
    { { -0.5f, 0.5f, 0.5f }, { 0,1,0 }, { 1.0f,0.f,1.f,1.f } },  // 16 
    { {  0.5f, 0.5f, 0.5f }, { 0,1,0 }, { 1.0f,0.f,1.f,1.f } },  // 17
    { { -0.5f, 0.5f,-0.5f }, { 0,1,0 }, { 1.0f,0.f,1.f,1.f } },  // 18
    { {  0.5f, 0.5f,-0.5f }, { 0,1,0 }, { 1.0f,0.f,1.f,1.f } },  // 19

    // bottom face
    { { -0.5f,-0.5f, 0.5f }, { 0,-1,0 }, { 0.0f,1.f,1.f,1.f } },  // 20 
    { {  0.5f,-0.5f, 0.5f }, { 0,-1,0 }, { 0.0f,1.f,1.f,1.f } },  // 21 
    { { -0.5f,-0.5f,-0.5f }, { 0,-1,0 }, { 0.0f,1.f,1.f,1.f } },  // 22 
    { {  0.5f,-0.5f,-0.5f }, { 0,-1,0 }, { 0.0f,1.f,1.f,1.f } },  // 23 
};

static const index_t cube_indices[] = {
    0,1,2, 3,2,1,

    4,6,5, 7,5,6,

    8,9,10, 10,11,8,

    12,15,14, 14,13,12,

    16,17,18, 18,17,19,

    22,23,21, 21,20,22 
};

#endif

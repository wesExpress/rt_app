cmake_minimum_required(VERSION 3.20)
SET(CMAKE_C_STANDARD 99)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

PROJECT(rt_app)

ADD_SUBDIRECTORY(DarkMatter)

SET(SOURCES 
    app.c
    camera.c
    entities.c

    rendering/nuklear_gui.c
    rendering/raster_pipeline.c
    rendering/rt_pipeline.c
    rendering/debug_pipeline.c
    rendering/quad_texture.c
)

ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES})

ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)

if(VULKAN)
    ADD_DEFINITIONS(-DDM_VULKAN)
endif()

TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PUBLIC -g -gcodeview -march=native)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC DarkMatter -fuse-ld=lld -Wl,/debug,/pdb:name.pdb)

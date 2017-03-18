#pragma once

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_thread.h>
#include "util/opengl.h"
#include <stdbool.h>

#define MAX_INTEGRAL 1024
// TODO: FIXME!
#define MAX_UDP_FRAME_SIZE 65536
#define MAX_UDP_FRAME_SIZE_STR "65536"

struct pattern {
    GLhandleARB * shader;
    int n_shaders;
    char * name;
    double intensity;
    double intensity_integral;

    int flip;
    GLuint * tex;
    GLuint fb;
    GLint * uni_tex;
    GLuint tex_output;

    GLuint image;

    SDL_Thread * network_thread;
    char * child_buffer;
};

int pattern_init(struct pattern * pattern, const char * prefix);
void pattern_term(struct pattern * pattern);
void pattern_render(struct pattern * pattern, GLuint input_tex);

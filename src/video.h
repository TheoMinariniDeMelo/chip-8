#pragma once

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>

#define CHIP8_WIDTH  64
#define CHIP8_HEIGHT 32

void clear_screen();
void init_sdl();
void draw_screen();

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "./video.h"

#define SAMPLE_RATE 44100
#define BEEP_FREQ   440.0

#define CHIP8_WIDTH  64
#define CHIP8_HEIGHT 32
#define SCALE 10
#define WIDTH_VIDEO (CHIP8_WIDTH * SCALE)
#define HEIGHT_VIDEO (CHIP8_HEIGHT * SCALE)

double phase = 0.0;

uint8_t sound_timer = 0;

uint8_t video[CHIP8_WIDTH * CHIP8_HEIGHT];

SDL_Window* window= NULL;
SDL_Renderer* renderer = NULL;


void clear_screen(){
    memset(video, 0, CHIP8_HEIGHT * CHIP8_WIDTH);
}
void draw_screen(){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for(int y = 0; y < CHIP8_HEIGHT; y++){
        for(int x = 0; x < CHIP8_WIDTH; x++){
            if(video[y * CHIP8_WIDTH + x]){
                SDL_Rect pixel = {
                    x * SCALE,
                    y * SCALE,
                    SCALE,
                    SCALE
                };
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

void audio_callback(void *userdata, Uint8 *stream, int len) {
    Sint16 *buffer = (Sint16 *)stream;
    int samples = len / sizeof(Sint16);

    for (int i = 0; i < samples; i++) {
        if (sound_timer > 0) {
            buffer[i] = (phase < 0.5) ? 8000 : -8000;
            phase += BEEP_FREQ / SAMPLE_RATE;
            if (phase >= 1.0)
                phase -= 1.0;
        } else {
            buffer[i] = 0;
        }
    }
}

void init_sdl(){
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_AudioSpec want, have;

    SDL_zero(want);

    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = audio_callback;

    if (SDL_OpenAudio(&want, &have) < 0) {
        SDL_Log("Erro no audio: %s", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow("CHIP-8 Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 420, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}

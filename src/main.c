#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "./fonts.h"

#define SAMPLE_RATE 44100
#define BEEP_FREQ   440.0

#define CHIP8_WIDTH  64
#define CHIP8_HEIGHT 32

uint8_t video[CHIP8_WIDTH * CHIP8_HEIGHT];

uint8_t arr[4096];
uint16_t stack[16];
uint8_t reg[16];
uint16_t I = 0;
uint8_t delay_timer = 0, sound_timer = 0;
uint16_t PC = 512;
uint8_t sp = 0;
Uint32 last_time;

double phase = 0.0;

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

void update_timers(Uint32 *last_time){
    Uint32 current = SDL_GetTicks();
    if((*last_time - current) >= (1000/60)) {
        if(delay_timer > 0) delay_timer--;
        if(sound_timer > 0) sound_timer--;
        *last_time = current;
    }
}

void load_rom(char* path){
    FILE* file = fopen(path, "ab+");
    uint8_t buffer[2];
    for(int i = 0 ; fread(buffer, 1,2, file) == 2; i += 2)
    {
        memmove(arr + i + 0x200, buffer, 2);
    }
}

uint16_t fetch(uint16_t address){
    uint16_t op = (arr[address] << 8 | arr[address + 1]);
    return op;
}
void push(uint16_t value){
    if(sp == 15) {
        perror("stack overflow 1");
        exit(1);
    }
    stack[++sp] = value;
}
uint16_t pop(){
    if(sp == 0){
        perror("stack underflow pop");
        exit(1);
    }
    return stack[sp--];
}

void execute(uint16_t op, SDL_Window *window, SDL_Renderer *renderer){
    uint8_t A = (op & 0xF000) >> 12;
    uint8_t B = (op & 0x0F00) >> 8;
    uint8_t C = (op & 0x00FF);
    uint16_t D = (op & 0x0FFF);
    switch (A) {
        case 0x0:
            switch(D){
                case 0x0E0:
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    return;
                case 0x0EE: 
                    PC = pop();
                    return;
                default:
                    push(PC);
                    PC = D;
                    return;
            }
        case 0x1:
            PC = D;
            return;
        case 0x2:
        case 0x3:
            if(reg[B] == C){
                PC += 2; // skip the next instruction
            }
            return;
        case 0x4:
            if(reg[B] != C){
                PC += 2; // skip the next instruction
            }
            return;
        case 0x5:
            if(reg[B] == reg[C >> 4]){
                PC += 2; // skip the next instruction
            }
            return;
        case 0x6:
            reg[B] = C;
            return;
        case 0x7:
            reg[B] += C;
            return;
        case 0x8:
            if((C & 0x0F) == 0){
                reg[B] = reg[C >> 4];
                return;
            }
            if((C & 0x0F) == 1){
                reg[B] |= reg[C >> 4];
                return;
            }
            if((C & 0x0F) == 2){
                reg[B] &= reg[C >> 4];
                return;
            }
            if((C & 0x0F) == 3){
                reg[B] ^= reg[C >> 4];
                return;
            }
            if((C & 0x0F) == 4){
                reg[B] += reg[C >> 4];
                return;
            }
            if((C & 0x0F) == 5){

                reg[B] -= reg[C >> 4];
                return;
            }
            if((C & 0x0F) == 6){
                reg[B] >>= reg[C >> 4];
                return;
            }
            if((C & 0x0F) == 7){
                reg[B] = reg[C >> 4] - reg[B];
                return;
            }
            if((C & 0x0F) == 0xE){
                reg[B] <<= reg[C >> 4];
                return;
            }
        case 0x9:
            if(reg[B] != reg[C >> 4]){
                PC += 1;
            };
            return;

        case 0xA:
            I = D;
            return;
        case 0xB:
            PC = reg[0] + D;
            return;
        case 0xC:
            reg[B] = (rand() % 256) & C;
            return;
        case 0xD:
            return;
        case 0xE:
            if(C == 0x9E){
                SDL_Event* event;
                while(1){
                    if(SDL_PollEvent(event)){
                        if(event->type == SDL_KEYDOWN){
                            if(event->key.keysym.sym == B){
                                PC += 2;
                            }
                            return;
                        }
                    }
                }
            }
            if(C == 0xA1){
                SDL_Event* event;
                while(1){
                    if(SDL_PollEvent(event)){
                        if(event->type == SDL_KEYDOWN){
                            if(event->key.keysym.sym != B){
                                PC += 2;
                            }
                            return;
                        }
                    }
                }
            }

        case 0xF:{
            if(C == 0x07){
                reg[B] = delay_timer;
                return;
            }
            if(C == 0x0A){
                SDL_Event *event;
                while(1){
                    if(SDL_PollEvent(event)){
                        if(event->type == SDL_KEYDOWN){
                            reg[B] = event->key.keysym.sym;
                            return;
                        }
                    }
                    update_timers(&last_time);
                }
            }
            if(C == 0x15){
                delay_timer = reg[B];
                return;
            }
            if(C == 0x1E){
                sound_timer = reg[B];
                return;
            }
            if(C == 0x1E){
                I += reg[B];
                return;
            }
            if(C == 0x29){
                I = 0x050 + (reg[B] * 5);
                return;
            }
            if(C == 0x33){
                int value = reg[B];
                arr[I] = value/100;
                arr[I + 1] = (value/10) % 10;
                arr[I + 2] = value % 10;
            }
            if(C == 0x55){
                for(int i = 0; i <= B; i++){
                    arr[I + i] = reg[i];
                }
            }
            if(C == 0x65){
                for(int i = 0; i <= B; i++){
                    reg[i] = arr[I + i];
                }
            }
        }
    }
}


int main(int argc, char **argv){
    if(argc != 2) 
    {
        perror("No ROM detected");
        return 1;
    }

    memcpy(arr + 0x050, chip8_font, 80);

    load_rom(argv[1]);
    SDL_Init(SDL_INIT_EVERYTHING);
    last_time = SDL_GetTicks();
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

    SDL_Window* window = SDL_CreateWindow("CHIP-8 Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 420, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    while (1) {
        SDL_Event event;
        /*while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT){
                exit(0);
            }
        }*/
        uint16_t op = fetch(PC);
        PC += 2;
        execute(op, window, renderer);


        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        update_timers(&last_time);
    }

}

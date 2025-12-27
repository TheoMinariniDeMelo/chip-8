#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "./fonts.h"
#include "./video.h"

#define CPU_HZ 500 // how quick the cpu will run

uint16_t keypad[16] = {
    SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5,
    SDLK_6, SDLK_7, SDLK_8, SDLK_9, SDLK_a, SDLK_b, 
    SDLK_c, SDLK_d, SDLK_e, SDLK_f
};

extern uint8_t sound_timer;
extern uint8_t video[CHIP8_WIDTH * CHIP8_HEIGHT];

Uint32 last_time;
uint16_t stack[16];
uint16_t I = 0;
uint16_t PC = 512;
uint8_t arr[4096];
uint8_t reg[16];
uint8_t delay_timer = 0;
uint8_t sp = 0;



int get_keypad_index(uint16_t sym){
    for(int i = 0; i < 16; i++){
        if(keypad[i] == sym) return i;
    }
    return -1;
}

void update_timers(Uint32 *last_time){
    Uint32 current = SDL_GetTicks();
    if((current - *last_time) >= (1000/60)) {
        if(delay_timer > 0) delay_timer--;
        if(sound_timer > 0) sound_timer--;
        *last_time = current;
    }
}

void load_rom(char* path){
    FILE* file = fopen(path, "rb");
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
        perror("stack overflow");
        exit(1);
    }
    stack[sp++] = value;
}
uint16_t pop(){
    if(sp == 0){
        perror("stack underflow");
        exit(1);
    }
    return stack[--sp];
}

void execute(uint16_t op){
    uint8_t A = (op & 0xF000) >> 12;
    uint8_t B = (op & 0x0F00) >> 8;
    uint8_t C = (op & 0x00FF);
    uint16_t D = (op & 0x0FFF);
    switch (A) {
        case 0x0:
            switch(D){
                case 0x0E0:
                    clear_screen();
                    return;
                case 0x0EE: 
                    PC = pop();
                    return;
                default:
                    PC = D;
                    return;
            }
        case 0x1:
            PC = D;
            return;
        case 0x2:
            push(PC);
            PC = D;
            return;
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
        case 0x8:{
            switch (C & 0x0F) {
                case 0x0:
                    reg[B] = reg[C >> 4];
                    return;

                case 0x1:
                    reg[B] |= reg[C >> 4];
                    return;

                case 0x2:
                    reg[B] &= reg[C >> 4];
                    return;

                case 0x3:
                    reg[B] ^= reg[C >> 4];
                    return;

                case 0x4:
                    reg[0xF] = (reg[B] + reg[C >> 4] > 0xFF);
                    reg[B] += reg[C >> 4];
                    return;

                case 0x5:
                    reg[0xF] = reg[B] >= reg[C >> 4];
                    reg[B] -= reg[C >> 4];
                    return;

                case 0x6:
                    reg[0xF] = reg[B] & 0x1;
                    reg[B] >>= 1;
                    return;

                case 0x7:
                    reg[0xF] = reg[C >> 4] >= reg[B];
                    reg[B] = reg[C >> 4] - reg[B];
                    return;

                case 0xE:
                    reg[0xF] = (reg[B] & 0x80) >> 7;
                    reg[B] <<= 1;
                    return;
            }
        }
        case 0x9:
            if(reg[B] != reg[C >> 4]){
                PC += 2;
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
        case 0xD:{
            uint8_t x0 = reg[B];
            uint8_t y0 = reg[C >> 4];
            uint8_t n = D & 0x000F;
            int x, y, index;
            reg[0xF] = 0;
            for(int row = 0; row < n; row++){
                uint8_t sprite = arr[I + row];
                for(int col = 0; col < 8; col++){
                    if(sprite & (0x80 >> col)){
                        x = (x0 + col) % CHIP8_WIDTH;
                        y = (y0 + row) % CHIP8_HEIGHT;
                        index = y * CHIP8_WIDTH + x;
                        if(video[index]){
                            reg[0xF] = 1;
                        }
                        video[index] ^= 1;
                    }
                }

            }
            return;
        }
        case 0xE:
            if(C == 0x9E){
                SDL_Event event;
                if(SDL_PollEvent(&event)){
                    if(event.type == SDL_KEYDOWN){
                        if(event.key.keysym.sym == keypad[reg[B]]){
                            PC += 2;
                        }
                        return;
                    }
                }
            }
            if(C == 0xA1){
                SDL_Event event;
                if(SDL_PollEvent(&event)){
                    if(event.type == SDL_KEYDOWN){
                        if(event.key.keysym.sym != keypad[reg[B]]){
                            PC += 2;
                        }
                        return;
                    }
                }
            }

        case 0xF:{
            if(C == 0x07){
                reg[B] = delay_timer;
                return;
            }
            if(C == 0x0A){
                SDL_Event event;
                if(SDL_PollEvent(&event)){
                    if(event.type == SDL_KEYDOWN){
                        int index = get_keypad_index(event.key.keysym.sym);
                        if(index != -1){
                            reg[B] = event.key.keysym.sym;
                            return;
                        }
                    }
                }
                PC -= 2;
                update_timers(&last_time);
            }
            if(C == 0x15){
                delay_timer = reg[B];
                return;
            }
            if(C == 0x18){
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

    last_time = SDL_GetTicks();

    init_sdl();

    const int instructions_per_frame = CPU_HZ / 60;

    while (1) {
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT){
                exit(0);
            }
        }
        Uint32 frame_start = SDL_GetTicks();

        for(int i = 0; i < instructions_per_frame; i++){    
            uint16_t op = fetch(PC);
            PC += 2;
            execute(op);

        }
        update_timers(&last_time);
        draw_screen();

        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < 16) {
            SDL_Delay(16 - frame_time);
        }
    }

}

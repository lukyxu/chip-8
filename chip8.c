//
// Created by Kerry Xu on 2019-09-10.
//

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include "chip8.h"
#include "disassembler.h"

chip8_t *instantiate_chip8(){
    unsigned char chip8_fontset[80] =
            {
                    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                    0x20, 0x60, 0x20, 0x20, 0x70, // 1
                    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
            };
    chip8_t *ret = calloc(1, sizeof(chip8_t));
    ret->memory = calloc(4096, sizeof(uint8_t));
    ret->registers = calloc(16, sizeof(uint8_t));
    ret->gfx = calloc(64*32, sizeof(uint8_t));
    ret->key = calloc(16, sizeof(bool));
    ret->stack = calloc(16, sizeof(uint16_t));
    ret->pc = 0x200u;

    for (int i = 0; i < 80; ++i) {
        ret->memory[i] = chip8_fontset[i];
    }
    return ret;
}

void load_game(char *file_path, chip8_t *chip8){
    FILE *file = fopen(file_path, "rb");
    if (file == NULL){
        printf("Could not open %s", file_path);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0L, SEEK_END);
    int fsize = ftell(file);
    fseek(file, 0L, SEEK_SET);
    fread(chip8->memory+0x200, fsize, 1, file);
    fclose(file);
}

int main (int argc, char** argv){
    chip8_t *chip8 = instantiate_chip8();
    if (argc < 2) {
        printf("Enter file path of ROM for second argument");
        exit(EXIT_FAILURE);
    }
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1){
        printf("Could not initialise SDL: %s\n", SDL_GetError());
    }
    SDL_Window *win = SDL_CreateWindow("CHIP8", 0, 0, 640, 320, 0);
    if (!win){
        printf("Could not create SDL Window: %s\n", SDL_GetError());
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    load_game(argv[1], chip8);
    int timer = 0;
    for(;;) {
        emulate_cycle(chip8);
        uint32_t startTime = SDL_GetTicks();
        if(chip8->draw_flag){
            draw_screen(chip8, renderer);
            SDL_RenderPresent(renderer);
        }
        handle_inputs(chip8);
        uint32_t frame_time = SDL_GetTicks() - startTime;
        if (CLOCK_PERIOD > frame_time){
            SDL_Delay(CLOCK_PERIOD - frame_time);
        }
        timer += CLOCK_PERIOD;
        if (timer > 1000/60) {
            if (chip8->delay_timer > 0) {
                chip8->delay_timer--;
            }
            if (chip8->sound_timer > 0) {
                if (chip8->sound_timer == 1) {
                    printf("Beep!");
                }
                chip8->sound_timer--;
            }
            timer = 0;
        }
    }
    SDL_DestroyWindow(win);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    return 0;
}

void emulate_cycle(chip8_t *chip8){

    // Fetch Instruction
    chip8->current_ins = (chip8->memory[chip8->pc] << 8u)| chip8->memory[chip8->pc + 1];
    printf("\n");
    disassemble_chip8(chip8->memory, chip8->pc);
    uint8_t nibble1 = (chip8->current_ins & 0xF000u) >> 12u;
    uint8_t nibble2 = (chip8->current_ins & 0xF00u) >> 8u;
    uint8_t nibble3 = (chip8->current_ins & 0xF0u) >> 4u;
    uint8_t nibble4 = chip8->current_ins & 0xFu;
    chip8->pc+=2;
    // Decode Instruction
    switch (nibble1){
        case 0x0:
            if (nibble3 == 0xE){
                if (nibble4 == 0x0){
                    // Execute Instruction - 00E0 - Clears the screen
                    memset(chip8->gfx, 0, sizeof(uint8_t) * 64 * 32);
                    chip8->draw_flag = true;
                }else if (nibble4 == 0xE){
                    // Execute Instruction - 00EE - Returns from a subroutine
                    if (chip8->stack_pointer == 0) {
                        return;
                    }
                    chip8->stack_pointer--;
                    chip8->pc = chip8->stack[chip8->stack_pointer];
                }else {
                    printf(NOT_IMPLEMENTED_ERROR);
                }
            }else {
                // Execute Instruction - 0NNN - Call RCA1802 program at address NNN
                printf(NOT_IMPLEMENTED_ERROR);
            }
            break;
        case 0x1:
            // Execute Instruction - 1NNN - Jump to address NNN
            chip8->pc = chip8->current_ins & 0xFFFu;
            break;
        case 0x2:
            // Execute Instruction - 2NNN - Calls subroutine at NNN
            chip8->stack[chip8->stack_pointer] = chip8->pc;
            chip8->stack_pointer++;
            chip8->pc = chip8->current_ins & 0xFFFu;
            break;
        case 0x3:
            // Execute Instruction - 3XNN - Skips the next instruction if VX == NN
            if (chip8->registers[nibble2] == (chip8->current_ins & 0xFFu)){
                chip8->pc += 2;
            }
            break;
        case 0x4:
            // Execute Instruction - 4XNN - Skips the next instruction if VX != NN
            if (chip8->registers[nibble2] != (chip8->current_ins & 0xFFu)){
                chip8->pc+=2;
            }
            break;
        case 0x5:
            if (nibble4 == 0){
                // Execute Instruction - 5XY0 - Skips the next instruction if VX == VY
                if (chip8->registers[nibble2] == chip8->registers[nibble3]){
                    chip8->pc += 2;
                }
            } else {
                printf(NOT_IMPLEMENTED_ERROR);
            }
            break;
        case 0x6:
            // Execute Instruction - 6XNN - Sets VX = NN
            chip8->registers[nibble2] = chip8->current_ins & 0xFFu;
            break;
        case 0x7:
            // Execute Instruction - 7XNN - Adds NN to VX (Carry flag not changed)
            chip8->registers[nibble2] += chip8->current_ins & 0xFFu;
            break;
        case 0x8:
            execute_opcode8(chip8);
            break;
        case 0x9:
            // Execute Instruction - 9XY0 - Skips the next instruction if VX != VY
            if (chip8->registers[nibble2] != chip8->registers[nibble3]){
                chip8->pc += 2;
            }
            break;
        case 0xA:
            // Execute Instruction - ANNN - Sets I to the address NNN
            chip8->i_reg = chip8->current_ins & 0xFFFu;
            break;
        case 0xB:
            // Execute Instruction - BNNN - Jumps to the address NNN plus V0
            chip8->pc = (chip8->current_ins + chip8->registers[0]) & 0xFFFu;
            break;
        case 0xC:
            // Execute Instruction - CXNN - Sets VX to the result of a bitwise and operation on a random number and NN
            chip8->registers[nibble2] = rand() & 0xFFu & chip8->current_ins;
            break;
        case 0xD:
            // Execute Instruction - DXYN - Draws a 8xN pixel sprite at coordinate (VX,VY)
            chip8->draw_flag = true;
            chip8->registers[15] = false;
                for (int j = 0; j < nibble4; j++) {
                    for (int i = 0; i < 8; i++) {
                        uint8_t pixel = chip8->memory[chip8->i_reg + j];
                        if (pixel >> (7u - i) & 0x1){
                            if (chip8->gfx[chip8->registers[nibble2] + i + 64 * (j+chip8->registers[nibble3])]){
                                chip8->registers[15] = true;
                            }
                            chip8->gfx[chip8->registers[nibble2]+ i + 64 * (j+chip8->registers[nibble3])] ^= 1;
                        }
                }
            }
            break;
        case 0xE:
            if (nibble3 == 0x9 && nibble4 == 0xE){
                // Execute Instruction - EX9E - Skips to next instruction if key stored in VX is pressed
                if (chip8->key[chip8->registers[nibble2]]){
                    chip8->pc +=2;
                    //chip8->key[chip8->registers[nibble2]] = false;
                }
                //memset(chip8->key, 0, 16);
            }else if (nibble3 == 0xA && nibble4 == 0x1){
                // Execute Instruction - EXA1 - Skips to next instruction if key stored in VX isn't pressed
                if (!chip8->key[chip8->registers[nibble2]]){
                    chip8->pc +=2;
                }
                //memset(chip8->key, 0, 16);
            }else {
                printf(NOT_IMPLEMENTED_ERROR);
            }
            break;
        case 0xF:
            execute_opcodeF(chip8);
            break;
        default:
            break;
    }
}


void execute_opcode8(chip8_t *chip8) {
    uint8_t nibble2 = (chip8->current_ins & 0xF00u) >> 8u;
    uint8_t nibble3 =  (chip8->current_ins & 0xF0u) >> 4u;
    uint8_t nibble4 =  chip8->current_ins & 0xFu;
    switch (nibble4){
        case 0x0:
            // Execute Instruction - 8XY0 - Sets VX to the value of VY
            chip8->registers[nibble2] = chip8->registers[nibble3];
            break;
        case 0x1:
            // Execute Instruction - 8XY1 - Sets VX to VX OR VY
            chip8->registers[nibble2] |= chip8->registers[nibble3];
            break;
        case 0x2:
            // Execute Instruction - 8XY2 - Sets VX to VX AND VY
            chip8->registers[nibble2] &= chip8->registers[nibble3];
            break;
        case 0x3:
            // Execute Instruction - 8XY3 - Sets VX to VX XOR VY
            chip8->registers[nibble2] ^= chip8->registers[nibble3];
            break;
        case 0x4:
            // Execute Instruction - 8XY4 - Adds VY to VX, VF set to 1 iff carry
            if (chip8->registers[nibble2] + chip8->registers[nibble3] < chip8->registers[nibble2]) {
                chip8->registers[15] = 1;
            } else {
                chip8->registers[15] = 0;
            }
            chip8->registers[nibble2] += chip8->registers[nibble3];
            break;
        case 0x5:
            // Execute Instruction - 8XY5 - Subtracts VY from VX, VF set to 0 iff borrow
            if (chip8->registers[nibble2] < chip8->registers[nibble3]) {
                chip8->registers[15] = 0;
            } else {
                chip8->registers[15] = 1;
            }
            chip8->registers[nibble2] -= chip8->registers[nibble3];
            break;
        case 0x6:
            // Execute Instruction - 8XY6 - Stores the least significant bit of VX in VF and VX >> 1
            chip8->registers[15] = chip8->registers[nibble2] & 0x1;
            chip8->registers[nibble2] >>= 1u;
            break;
        case 0x7:
            // Execute Instruction - 8XY7 - Stores VX to VY minus VX, VF set to 0 iff borrow
            if (chip8->registers[nibble3] < chip8->registers[nibble2]) {
                chip8->registers[15] = 0;
            } else {
                chip8->registers[15] = 1;
            }
            chip8->registers[nibble2] = chip8->registers[nibble3] - chip8->registers[nibble2];
            break;
        case 0xE:
            // Execute Instruction - 8XYE - Stores the most significant bit of VX in VF and VX << 1
            chip8->registers[15] = chip8->registers[nibble2] >> 7u;
            chip8->registers[nibble2] <<= 1u;
            break;
        default:
            printf(NOT_IMPLEMENTED_ERROR);
    }
}

void execute_opcodeF(chip8_t *chip8) {
    uint8_t nibble2 = (chip8->current_ins & 0xF00u) >> 8u;
    uint8_t byte2 =  chip8->current_ins & 0xFFu;
    switch (byte2){
        case 0x07:
            // Execute Instruction - FX07 - Sets VX to the value of the delay timer
            chip8->registers[nibble2] = chip8->delay_timer;
            break;
        case 0x0A:
            // Execute Instruction - FX0A - A key press is awaited and then stored in VX (Blocking Operation)
        {
            //SDL_Event event;
            bool key_pressed = false;
            for (int i = 0; i < 16; ++i) {
                if (chip8->key[i]) {
                    key_pressed = true;
                    chip8->registers[nibble2] = i;
                    break;
                }
            }
            if (!key_pressed) {
                chip8->pc -= 2;
            }
//            while (SDL_PollEvent(&event)) {
//                if (event.type == SDL_KEYDOWN){
//                        switch (event.key.keysym.sym) {
//                            case SDLK_1:
//                                chip8->registers[nibble2] = 1;
//                                break;
//                            case SDLK_2:
//                                chip8->registers[nibble2] = 2;
//                                break;
//                            case SDLK_3:
//                                chip8->registers[nibble2] = 3;
//                                break;
//                            case SDLK_4:
//                                chip8->registers[nibble2] = 12;
//                                break;
//                            case SDLK_q:
//                                chip8->registers[nibble2] = 4;
//                                break;
//                            case SDLK_w:
//                                chip8->registers[nibble2] = 5;
//                                break;
//                            case SDLK_e:
//                                chip8->registers[nibble2] = 6;
//                                break;
//                            case SDLK_r:
//                                chip8->registers[nibble2] = 13;
//                                break;
//                            case SDLK_a:
//                                chip8->registers[nibble2] = 7;
//                                break;
//                            case SDLK_s:
//                                chip8->registers[nibble2] = 8;
//                                break;
//                            case SDLK_d:
//                                chip8->registers[nibble2] = 9;
//                                break;
//                            case SDLK_f:
//                                chip8->registers[nibble2] = 14;
//                                break;
//                            case SDLK_z:
//                                chip8->registers[nibble2] = 10;
//                                break;
//                            case SDLK_x:
//                                chip8->registers[nibble2] = 0;
//                                break;
//                            case SDLK_c:
//                                chip8->registers[nibble2] = 11;
//                                break;
//                            case SDLK_v:
//                                chip8->registers[nibble2] = 15;
//                                break;
//                        }
//                        key_pressed = true;
//                }
//            }
        }
        case 0x15:
            // Execute Instruction - FX15 - Sets the delay timer to VX
            chip8->delay_timer = chip8->registers[nibble2];
            break;
        case 0x18:
            // Execute Instruction - FX18 - Sets the sound timer to VX
            chip8->sound_timer = chip8->registers[nibble2];
            break;
        case 0x1E:
            // Execute Instruction - FX1E - Adds VX to I
            chip8->i_reg += chip8->registers[nibble2];
            break;
        case 0x29:
            // Execute Instruction - FX29 - Sets I to the location of sprite for the character in VX
            chip8->i_reg = (chip8->registers[nibble2] & 0xFu) * 5;
            break;
        case 0x33:
            // Execute Instruction - FX33 - Stores the BCD representation of VX at address I
            chip8->memory[chip8->i_reg] = chip8->registers[nibble2] / 100;
            chip8->memory[chip8->i_reg+1] = (chip8->registers[nibble2] / 10) % 10;
            chip8->memory[chip8->i_reg+2] = (chip8->registers[nibble2] % 10);
            break;
        case 0x55:
            // Execute Instruction - FX55 - Stores V0 to VX (inclusive) in memory starting at address I
            for (int i = 0; i <= nibble2; i++) {
                chip8->memory[chip8->i_reg + i] = chip8->registers[i];
            }
            break;
        case 0x65:
            // Execute Instruction - FX65 - Fills V0 to VX (inclusive) with values from memory starting at memory I
            for (int i = 0; i <= nibble2; i++) {
                chip8->registers[i] = chip8->memory[chip8->i_reg + i];
            }
            break;
        default:
            printf(NOT_IMPLEMENTED_ERROR);
    }
}

void draw_screen(chip8_t *chip8, SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int j = 0; j < 32; j++) {
        for (int i = 0; i < 64; i++) {
            if (chip8->gfx[i + (64*j)]){
                SDL_Rect rect = (SDL_Rect) {.x = i*10, .y = j*10, .w = 10, .h = 10};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    chip8->draw_flag = false;
}

void handle_inputs(chip8_t *chip8) {
    // Keyboard mappings
    // 1 -> 1, 2 -> 2, 3 -> 3, 4 -> C
    // Q -> 4, W -> 5, E -> 6, R -> D
    // A -> 7, S -> 8, D -> 8, F -> E
    // Z -> A, X -> 0, C -> B, V -> F
    SDL_Event event;
    while (SDL_PollEvent(&event)){
        switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_1:
                        chip8->key[1] = true;
                        break;
                    case SDLK_2:
                        chip8->key[2] = true;
                        break;
                    case SDLK_3:
                        chip8->key[3] = true;
                        break;
                    case SDLK_4:
                        chip8->key[12] = true;
                        break;
                    case SDLK_q:
                        chip8->key[4] = true;
                        break;
                    case SDLK_w:
                        chip8->key[5] = true;
                        break;
                    case SDLK_e:
                        chip8->key[6] = true;
                        break;
                    case SDLK_r:
                        chip8->key[13] = true;
                        break;
                    case SDLK_a:
                        chip8->key[7] = true;
                        break;
                    case SDLK_s:
                        chip8->key[8] = true;
                        break;
                    case SDLK_d:
                        chip8->key[9] = true;
                        break;
                    case SDLK_f:
                        chip8->key[14] = true;
                        break;
                    case SDLK_z:
                        chip8->key[10] = true;
                        break;
                    case SDLK_x:
                        chip8->key[0] = true;
                        break;
                    case SDLK_c:
                        chip8->key[11] = true;
                        break;
                    case SDLK_v:
                        chip8->key[15] = true;
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_1:
                        chip8->key[1] = false;
                        break;
                    case SDLK_2:
                        chip8->key[2] = false;
                        break;
                    case SDLK_3:
                        chip8->key[3] = false;
                        break;
                    case SDLK_4:
                        chip8->key[12] = false;
                        break;
                    case SDLK_q:
                        chip8->key[4] = false;
                        break;
                    case SDLK_w:
                        chip8->key[5] = false;
                        break;
                    case SDLK_e:
                        chip8->key[6] = false;
                        break;
                    case SDLK_r:
                        chip8->key[13] = false;
                        break;
                    case SDLK_a:
                        chip8->key[7] = false;
                        break;
                    case SDLK_s:
                        chip8->key[8] = false;
                        break;
                    case SDLK_d:
                        chip8->key[9] = false;
                        break;
                    case SDLK_f:
                        chip8->key[14] = false;
                        break;
                    case SDLK_z:
                        chip8->key[10] = false;
                        break;
                    case SDLK_x:
                        chip8->key[0] = false;
                        break;
                    case SDLK_c:
                        chip8->key[11] = false;
                        break;
                    case SDLK_v:
                        chip8->key[15] = false;
                        break;
                }
                break;
        }
    }
}
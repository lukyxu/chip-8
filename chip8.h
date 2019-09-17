//
// Created by Kerry Xu on 2019-09-10.
//

#ifndef CHIP_8_EMULATOR_CHIP8_H
#define CHIP_8_EMULATOR_CHIP8_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#define CLOCK_SPEED 400
#define CLOCK_PERIOD (1000/CLOCK_SPEED)
typedef struct chip8 {
    uint16_t current_ins;
    uint16_t pc;
    uint16_t stack_pointer;
    uint16_t i_reg;
    uint8_t *memory;
    uint8_t *registers;
    uint8_t *gfx;
    bool *key;
    uint16_t *stack;

    uint8_t delay_timer;
    uint8_t sound_timer;
    bool draw_flag;
}chip8_t;
void emulate_cycle(chip8_t *chip8);
void execute_opcode8(chip8_t *chip8);
void execute_opcodeF(chip8_t *chip8);
void draw_screen(chip8_t *chip8, SDL_Renderer *renderer);
void handle_inputs(chip8_t *chip8);
#endif //CHIP_8_EMULATOR_CHIP8_H

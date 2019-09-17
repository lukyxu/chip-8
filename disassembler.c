//
// Created by Kerry Xu on 2019-09-04.
//

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "disassembler.h"

//int main (int argc, char** argv){
//    FILE *file = fopen(argv[1], "rb");
//    if (file == NULL){
//        printf("Could not open %s", argv[1]);
//        exit(EXIT_FAILURE);
//    }
//    fseek(file, 0L, SEEK_END);
//    int fsize = ftell(file);
//    fseek(file, 0L, SEEK_SET);
//    uint8_t *buffer = malloc(fsize+0x200);
//    fread(buffer+0x200, fsize, 1, file);
//    fclose(file);
//    int pc = 0x200u;
//    while (pc < 0x200+fsize){
//        disassemble_chip8(buffer, pc);
//        pc += 2;
//        printf("\n");
//    }
//    return EXIT_SUCCESS;
//}

void disassemble_chip8(uint8_t *memory, int pc) {
    uint8_t *instruction = &memory[pc];
    uint8_t nibble1 = instruction[0] >> 4u;
    uint8_t nibble2 = instruction[0] & 0xFu;
    uint8_t nibble3 = instruction[1] >> 4u;
    uint8_t nibble4 = instruction[1] & 0xFu;
    printf("%04X %02X %02X ", pc, instruction[0], instruction[1]);
    switch (nibble1){
        case 0x0:
            if (nibble3 == 0xE){
                if (nibble4 == 0x0){
                    printf("%-10s", "CLS");
                }else if (nibble4 == 0xE){
                    printf("%-10s", "RTS");
                }else {
                    printf(NOT_IMPLEMENTED_ERROR);
                }
            }else {
                printf("%-10s #%04X", "SYS", get_address(instruction));
            }
            break;
        case 0x1:
            printf("%-10s #%04X", "JP", get_address(instruction));
            break;
        case 0x2:
            printf("%-10s #%04X", "CALL", get_address(instruction));
            break;
        case 0x3:
            printf("%-10s V%1X, #%02X", "SE", nibble2, instruction[1]);
            break;
        case 0x4:
            printf("%-10s V%1X, #%02X", "SNE", nibble2, instruction[1]);
            break;
        case 0x5:
            if (nibble4 == 0){
                printf("%-10s V%1X, V%1X", "SNE", nibble2, nibble3);
            } else {
                printf(NOT_IMPLEMENTED_ERROR);
            }
            break;
        case 0x6:
            printf("%-10s V%1X, #%02X", "LD", nibble2, instruction[1]);
            break;
        case 0x7:
            printf("%-10s V%1X, #%02X", "ADD", nibble2, instruction[1]);
            break;
        case 0x8:
            disassemble_opcode8(nibble2, nibble3, nibble4);
            break;
        case 0x9:
            printf("%-10s V%1X, V%1X", "SNE", nibble2, nibble3);
            break;
        case 0xA:
            printf("%-10s I, #%04X", "LD", get_address(instruction));
            break;
        case 0xB:
            printf("%-10s V0, #%04X", "JP", get_address(instruction));
            break;
        case 0xC:
            printf("%-10s V%1X, #%02X", "RND", nibble2, instruction[1]);
            break;
        case 0xD:
            printf("%-10s V%1X, V%1X, %1X", "DRW", nibble2, nibble3, nibble4);
            break;
        case 0xE:
            if (nibble3 == 0x9 && nibble4 == 0xE){
                printf("%-10s V%1X", "SKP", nibble2);
            }else if (nibble3 == 0xA && nibble4 == 0x1){
                printf("%-10s V%1X", "SKNP", nibble2);
            }else {
                printf(NOT_IMPLEMENTED_ERROR);
            }
            break;
        case 0xF:
            disassemble_opcodeF(nibble2, instruction[1]);
            break;
        default:
            break;
    }
}

uint16_t get_address(const uint8_t *instruction) {
    return (instruction[0] & 0xFu) << 8u | instruction[1];
}

void disassemble_opcode8(uint8_t nibble2, uint8_t nibble3, uint8_t nibble4) {
    switch (nibble4){
        case 0x0:
            printf("%-10s V%1X, V%1X", "LD", nibble2, nibble3);
            break;
        case 0x1:
            printf("%-10s V%1X, V%1X", "OR", nibble2, nibble3);
            break;
        case 0x2:
            printf("%-10s V%1X, V%1X", "AND", nibble2, nibble3);
            break;
        case 0x3:
            printf("%-10s V%1X, V%1X", "XOR", nibble2, nibble3);
            break;
        case 0x4:
            printf("%-10s V%1X, V%1X", "ADD", nibble2, nibble3);
            break;
        case 0x5:
            printf("%-10s V%1X, V%1X", "SUB", nibble2, nibble3);
            break;
        case 0x6:
            printf("%-10s V%1X, V%1X", "SHR", nibble2, nibble3);
            break;
        case 0x7:
            printf("%-10s V%1X, V%1X", "SUBN", nibble2, nibble3);
            break;
        case 0xE:
            printf("%-10s V%1X, V%1X", "SHL", nibble2, nibble3);
            break;
        default:
            printf(NOT_IMPLEMENTED_ERROR);

    }
}

void disassemble_opcodeF(uint8_t nibble2, uint8_t byte2) {
    switch (byte2){
        case 0x0A:
            printf("%-10s V%1X, K", "LD", nibble2);
            break;
        case 0x07:
            printf("%-10s V%1X, DT", "LD", nibble2);
            break;
        case 0x15:
            printf("%-10s DT, V%1X", "LD", nibble2);
            break;
        case 0x18:
            printf("%-10s ST, V%1X", "LD", nibble2);
            break;
        case 0x29:
            printf("%-10s F, V%1X", "LD", nibble2);
            break;
        case 0x33:
            printf("%-10s V%1X", "BCD", nibble2);
            break;

        case 0x55:
            printf("%-10s [I], V%1X", "LD", nibble2);
            break;
        case 0x65:
            printf("%-10s V%1X, [I]", "LD", nibble2);
            break;
        case 0x1E:
            printf("%-10s I, V%1X", "ADD", nibble2);
            break;
        default:
            printf(NOT_IMPLEMENTED_ERROR);
    }
}
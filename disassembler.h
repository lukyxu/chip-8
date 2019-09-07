//
// Created by Kerry Xu on 2019-09-04.
//

#ifndef CHIP_8_EMULATOR_DISASSEMBLER_H
#define CHIP_8_EMULATOR_DISASSEMBLER_H
#define NOT_IMPLEMENTED_ERROR "Super CHIP-8 instruction disassembly not implemented"
void disassemble_chip8(uint8_t *memory, int pc);
void disassemble_opcode8(uint8_t nibble2, uint8_t nibble3, uint8_t nibble4);
void disassemble_opcodeF(uint8_t nibble2, uint8_t byte2);
uint16_t get_address(const uint8_t *instruction);
#endif //CHIP_8_EMULATOR_DISASSEMBLER_H

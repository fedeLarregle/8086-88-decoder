#include <stdio.h>
#include <stdint.h>

/*
    NOTE(fede): mov register/register OPCODE information
    8 bit      | 8 bit
    100010 D W | mod | reg | r/m
    100010 (6 bits) = OPCODE (mov in this case)
    D      (1 bit)  = if 0 _reg_ field is the source if 1 _reg_ field is the destination
    W      (1 bit)  = "is this 16 bits or not" (wide bit) if 0 8 bits else 16 bits
    mod    (2 bits) = encodes what type of mov we are performing (e.g.: register to register)
    reg    (3 bits) = encodes a register
    r/m    (3 bits) = encodes a register (or memory depending of the value of mod)

    e.g.:
    mov Ax, Bx (16 bits)
    mov Al, Bl (low 8 bits)
    mov Ah, Bh (high 8 bits)

    Task:
    1. Read bits
    2. Print the asm instructions
    */

bool check_pattern(uint8_t byte, uint8_t pattern, uint8_t mask) {
    return (byte & mask) == pattern;
}

const char * reg_rm[16] = {
    [0]  = "ax", [1]  = "cx", [2]  = "dx",
    [3]  = "bx", [4]  = "sp", [5]  = "bp",
    [6]  = "si", [7]  = "di", [8]  = "al",
    [9]  = "cl", [10] = "dl", [11] = "bl",
    [12] = "ah", [13] = "ch", [14] = "dh",
    [15] = "bh"
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "[ERROR] Missing filename argument.\n");
        return 1;
    }

    const char *filename = argv[1];
    FILE *file = fopen(filename, "rb");

    if (!file) {
        fprintf(stderr, "[ERROR] Error opening file with filename = %s\n", filename);
        return 1;
    }

    printf("; %s:\n", filename);
    printf("bits 16\n\n");

    unsigned char buffer[1024];
    uint8_t byte;
    uint8_t mov_pattern = 0b100010;
    uint8_t mov_mask = 0b111111;

    while ((fread(&byte, 1, 1, file))) {
        // NOTE(fede): Check if we have found a mov
        // instruction without considering D W bits
        if (check_pattern(byte >> 2, mov_pattern, mov_mask)) {
            // NOTE(fede): Check D bit
            bool is_reg_destination = (byte & 0b00000010) == 0b00000010;
            bool is_wide_register = (byte & 0b00000001) == 0b00000001;
            uint8_t wide_offset = 0;
            if (!is_wide_register) {
                wide_offset += 8;
            }

            size_t read_byte = fread(&byte, 1, 1, file);
            if (!read_byte) {
                fprintf(stderr, "[ERROR] wrong MOV encoding.\n");
                return 1;
            }

            bool register_mode = ((byte >> 6) & 0b00000011) == 0b00000011;

            const char *reg = reg_rm[((byte >> 3) & 0b00000111) + wide_offset];
            const char *rm = reg_rm[((byte) & 0b00000111) + wide_offset];

            if (is_reg_destination) {
                printf("mov %s, %s\n", reg, rm);
            } else {
                printf("mov %s, %s\n", rm, reg);
            }
        }
    }

    fclose(file);

    return 0;
}
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
    // NOTE(fede): 1011 | W | REG | LO | HI (e.g.: mov ax, 12)
    // if is not W (0) L bits will follow otherwise
    // it will need to load the high bits as well
    uint8_t mov_imm_to_reg_pattern = 0b100010;
    uint8_t mov_imm_to_reg_mask = 0b111111;
    // NOTE(fede): There is also mov immidiate to memory
    // e.g.: mov [BP + 75], 12
    // that requires extra information to know wether is
    // an 8 bit or a 16 bit mov. For that we include extra
    // keywords as follows: mov [BP + 75], byte/word 12 (8bits/16bits respectively)

    while ((fread(&byte, 1, 1, file))) {
        if (check_pattern(byte >> 2, mov_pattern, mov_mask)) {
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

            // NOTE(fede): type of mov (e.g.: reg/reg - reg/mem - mem/reg) not used for now
            uint8_t register_mode = ((byte >> 6) & 0b00000011);
            const uint8_t mod_reg_to_reg = 0b00000011;
            // NOTE(fede): Does have an 8 bit displacement            (e.g.: [... + 8])
            const uint8_t mod_8_bit_disp = 0b00000001;
            // NOTE(fede): Does have an 16 bit displacement           (e.g.: [... + 16])
            const uint8_t mod_16_bit_disp = 0b00000010;
            // NOTE(fede): Does _not_ regenerally have a displacement (e.g.: [...])
            // we also need to check the r/m field, if it is set to 110 then it is
            // a direct address, and we have a 16 bits displacement.
            const uint8_t mod_no_disp_ = 0b00000000;

            switch (register_mode) {
                case mod_reg_to_reg: {
                    const char *reg = reg_rm[((byte >> 3) & 0b00000111) + wide_offset];
                    const char *rm = reg_rm[((byte) & 0b00000111) + wide_offset];

                    if (is_reg_destination) {
                        printf("mov %s, %s\n", reg, rm);
                    } else {
                        printf("mov %s, %s\n", rm, reg);
                    }
                    break;
                }
                case mod_8_bit_disp: {
                    break;
                }
                case mod_16_bit_disp: {
                    break;
                }
                case mod_no_disp_: {
                    break;
                }
                // TODO(fede): read DL (displacement low) DH (displacement high)
                // EFFECTIVE ADDRESS CALCULATION
                /*
                R/M      MOD (00)        MOD (01)            MOD (10)
                000     (BX) + (SI)     (BX) + (SI) + D8    (BX) + (SI) + D16
                001     (BX) + (DI)     (BX) + (DI) + D8    (BX) + (DI) + D16
                010     (BP) + (SI)     (BP) + (SI) + D8    (BP) + (SI) + D16
                011     (BP) + (DI)     (BP) + (DI) + D8    (BP) + (DI) + D16
                100     (SI)            (SI) + D8           (SI) + D16
                101     (DI)            (DI) + D8           (DI) + D16
                110     DIRRECT ADDRESS (BP) + D8           (BP) + D16
                111     (BX)            (BX) + D8           (BX) + D16
                */
                default: {
                    fprintf(stderr, "[ERROR] not supported MOV register_mode 0x%02X\n", register_mode);
                    return 1;
                }
            }
        }
    }

    fclose(file);

    return 0;
}
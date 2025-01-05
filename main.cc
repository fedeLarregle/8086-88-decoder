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

// NOTE(fede): Intel 8086 Manual: EFFECTIVE ADDRESS CALCULATION
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
const char * effective_addr_calculation[3][8] = {
    // MOD = 00
    [0][0] = "[bx + si]",      [0][1] = "[bx + di]",      [0][2] = "[bp + si]",      [0][3] = "[bp + di]",
    [0][4] = "[si]",           [0][5] = "[di]",           [0][6] = "[%d]",           [0][7] = "[bx]",
    // MOD = 01
    [1][0] = "[bx + si + %d]", [1][1] = "[bx + di + %d]", [1][2] = "[bp + si + %d]", [1][3] = "[bp + di + %d]",
    [1][4] = "[si + %d]",      [1][5] = "[di + %d]",      [1][6] = "[bp + %d]",      [1][7] = "[bx + %d]",
    // MOD = 10
    [2][0] = "[bx + si + %d]", [2][1] = "[bx + di + %d]", [2][2] = "[bp + si + %d]", [2][3] = "[bp + di + %d]",
    [2][4] = "[si + %d]",      [2][5] = "[di + %d]",      [2][6] = "[bp + %d]",      [2][7] = "[bx + %d]", 
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

    while ((fread(&byte, 1, 1, file))) {
        // NOTE(fede): MOV Register/memory to/from register
        if (((byte >> 2) & 0b111111) == 0b100010) {
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

            const char *reg = reg_rm[((byte >> 3) & 0b00000111) + wide_offset];
            uint8_t rm = ((byte) & 0b00000111);

            uint8_t register_mode = ((byte >> 6) & 0b00000011);
            const uint8_t mod_reg_to_reg = 0b00000011;
            const uint8_t mod_8_bit_disp = 0b00000001;
            const uint8_t mod_16_bit_disp = 0b00000010;
            const uint8_t mod_no_disp_ = 0b00000000;
            
            switch (register_mode) {
                
                case mod_reg_to_reg: {
                    const char *rm_reg = reg_rm[(rm + wide_offset)];

                    if (is_reg_destination) {
                        printf("mov %s, %s\n", reg, rm_reg);
                    } else {
                        printf("mov %s, %s\n", rm_reg, reg);
                    }
                    break;
                }
                case mod_8_bit_disp: {
                    const char *calc = effective_addr_calculation[mod_8_bit_disp][rm];
                    size_t read_byte = fread(&byte, 1, 1, file);
                    if (!read_byte) {
                        fprintf(stderr, "[ERROR] wrong MOV encoding.\n");
                        return 1;
                    }
                    if (is_reg_destination) {
                        printf("mov %s, ", reg);
                        printf(calc, byte);
                        printf("\n");
                    } else {
                        printf("mov ");
                        printf(calc, byte);
                        printf(", %s\n", reg);
                    }
                    break;
                }
                case mod_16_bit_disp: {
                    const char *calc = effective_addr_calculation[mod_16_bit_disp][rm];
                    uint16_t b16;
                    size_t read_byte = fread(&b16, sizeof(uint16_t), 1, file);
                    if (!read_byte) {
                        fprintf(stderr, "[ERROR] wrong MOV encoding.\n");
                        return 1;
                    }

                    if (is_reg_destination) {
                        printf("mov %s, ", reg);
                        printf(calc, b16);
                        printf("\n");
                    } else {
                        printf("mov ");
                        printf(calc, b16);
                        printf(", %s\n", reg);
                    }

                    break;
                }
                case mod_no_disp_: {
                    const char *calc = effective_addr_calculation[mod_no_disp_][rm];
                    uint16_t b16;
                    if (rm == 0b00000110) {
                        size_t read_byte = fread(&b16, sizeof(uint16_t), 1, file);
                        if (!read_byte) {
                            fprintf(stderr, "[ERROR] wrong MOV encoding.\n");
                            return 1;
                        }

                        if (is_reg_destination) {
                            printf("mov %s, ", reg);
                            printf(calc, b16);
                            printf("\n");
                        } else {
                            printf("mov ");
                            printf(calc, b16);
                            printf(", %s\n", reg);
                        }
                    } else {
                        if (is_reg_destination) {
                            printf("mov %s, ", reg);
                            printf(calc, b16);
                            printf("\n");
                        } else {
                            printf("mov ");
                            printf(calc, b16);
                            printf(", %s\n", reg);
                        }
                    }
                    break;
                }
                default: {
                    fprintf(stderr, "[ERROR] not supported MOV register_mode 0x%02X\n", register_mode);
                    return 1;
                }
            } 
        } else if (((byte >> 4) & 0b1111) == 0b1011) {
            // NOTE(fede): MOV immediate to register
            bool is_wide_register = (byte & 0b00001000) == 0b00001000;
            uint8_t wide_offset = is_wide_register ? 0b00000000 : 0b00001000;
            
            const char *reg = reg_rm[((byte) & 0b00000111) + wide_offset];

            if (is_wide_register) {
                uint16_t b16;
                size_t read_byte = fread(&b16, sizeof(uint16_t), 1, file);
                if (!read_byte) {
                    fprintf(stderr, "[ERROR] wrong MOV encoding.\n");
                    return 1;
                }
                printf("mov %s, %d\n", reg, b16);
            } else {
                size_t read_byte = fread(&byte, 1, 1, file);
                if (!read_byte) {
                    fprintf(stderr, "[ERROR] wrong MOV encoding.\n");
                    return 1;
                }
                printf("mov %s, %d\n", reg, byte);
            }
        }
        // TODO(fede): Add immediate to register
        // e.g.: mov [BP + 75], 12
        // that requires extra information to know wether is
        // an 8 bit or a 16 bit mov. For that we include extra
        // keywords as follows: mov [BP + 75], byte/word 12 (8bits/16bits respectively)
    }

    fclose(file);

    return 0;
}
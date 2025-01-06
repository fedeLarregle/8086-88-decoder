#include <stdio.h>
#include <stdint.h>

const char * effective_addr_calculation[3][8] = {
    // MOD = 00
    [0][0] = "[bx + si]",      [0][1] = "[bx + di]",      [0][2] = "[bp + si]",      [0][3] = "[bp + di]",
    [0][4] = "[si]",           [0][5] = "[di]",           [0][6] = "[%d]",           [0][7] = "[bx]",
    // MOD = 01
    [1][0] = "[bx + si + %d]", [1][1] = "[bx + di + %d]", [1][2] = "[bp + si + %d]", [1][3] = "[bp + di + %d]",
    [1][4] = "[si + %d]",      [1][5] = "[di + %d]",      [1][6] = "[bp + %d]",      [1][7] = "[bx + %d]",
    // MOD = 10
    [2][0] = "[bx + si + %d]", [2][1] = "[bx + di + %d]", [2][2] = "[bp + si + %d]", [2][3] = "[bp + di + %d]",
    [2][4] = "[si + %d]",      [2][5] = "[di + %d]",      [2][6] = "[bp + %d]",      [2][7] = "[bx + %d]"
};

const char * reg_rm_11[2][8] = {
    // W = 0
    [0][0] = "al", [0][1] = "cl", [0][2] = "dl", [0][3] = "bl",
    [0][4] = "ah", [0][5] = "ch", [0][6] = "dh", [0][7] = "bh",
    // W = 1
    [1][0] = "ax", [1][1] = "cx", [1][2] = "dx", [1][3] = "bx",
    [1][4] = "sp", [1][5] = "bp", [1][6] = "si", [1][7] = "di"
};

enum mov_mode {
    rm_to_rm,
    imm_to_rm,
    imm_to_r,
    mem_to_acc,
    acc_to_mem,
    rm_to_seg,
    seg_to_rm
};

struct mov_opcode {
    mov_mode mode;
    uint8_t d;
    uint8_t w;
    uint8_t mod;
    uint8_t reg;
    uint8_t rm;
    uint8_t disp_low;
    uint8_t disp_high;
    uint8_t data_low;
    uint8_t data_high;
};

bool matches_mov_opcode(uint8_t byte, mov_mode *mov) {
    bool result = false;
    if (((byte >> 2) & 0b111111) == 0b100010) {
        *mov = rm_to_rm;
        result = true;
    } else if (((byte >> 2) & 0b1111111) == 0b1100011) {
        *mov = imm_to_rm;
        result = true;
    } else if (((byte >> 4) & 0b1111) == 0b1011) {
        *mov = imm_to_r;
        result = true;
    } else if (((byte >> 1) & 0b1111111) == 0b1010000) {
        *mov = mem_to_acc;
        result = true;
    } else if (((byte >> 1) & 0b1111111) == 0b1010001) {
        *mov = acc_to_mem;
        result = true;
    } else if (((byte) & 0b11111111) == 0b10001110) {
        *mov = rm_to_seg;
        result = true;
    } else if (((byte) & 0b11111111) == 0b10001100) {
        *mov = seg_to_rm;
        result = true;
    }

    return result;
}

void print_mov(mov_opcode * opcode, uint16_t data) {
    const char * calc = effective_addr_calculation[opcode->mod][opcode->rm];
    const char * reg = reg_rm_11[opcode->w][opcode->reg];
    if (opcode->d) {
        printf("mov %s, ", reg);
        printf(calc, data);
        printf("\n");
    } else {
        printf("mov ");
        printf(calc, ((uint8_t)(data >> 8)));
        printf(", %s\n", reg);
    }
}

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

    uint8_t byte;

    while ((fread(&byte, 1, 1, file))) {
        mov_mode mode;
        if (matches_mov_opcode(byte, &mode)) {
            mov_opcode opcode = {};
            opcode.mode = mode;

            switch (opcode.mode) {
                case rm_to_rm: {
                    opcode.d = (byte & 0b00000010);
                    opcode.w = (byte & 0b00000001);
                    size_t read_byte = fread(&byte, 1, 1, file);
                    if (!read_byte) { return 1; }

                    opcode.mod = ((byte >> 6) & 0b00000011);
                    opcode.reg = ((byte >> 3) & 0b00000111);
                    opcode.rm  = ((byte >> 0) & 0b00000111);

                    switch (opcode.mod) {
                        case 0b00000000: {
                            char * rm_code;
                            if (opcode.rm == 0b00000110) {
                                read_byte = fread(&byte, 1, 1, file);
                                if (!read_byte) { return 1; }

                                opcode.disp_low = byte;

                                read_byte = fread(&byte, 1, 1, file);
                                if (!read_byte) { return 1; }

                                opcode.disp_high = byte;

                                print_mov(&opcode, (((uint16_t) opcode.disp_high << 8) | opcode.disp_low));
                                break;
                            }

                            const char * calc = effective_addr_calculation[opcode.mod][opcode.rm];
                            const char * reg  = reg_rm_11[opcode.w][opcode.reg];
                            if (opcode.d) {
                                printf("mov %s, %s\n", reg, calc);
                            } else {
                                printf("mov %s, %s\n", calc, reg);
                            }

                            break;
                        }
                        case 0b00000001: {  /* 8 bit displacement */
                            read_byte = fread(&byte, 1, 1, file);
                            if (!read_byte) { return 1; }

                            opcode.disp_low = byte;

                            print_mov(&opcode, (((uint16_t) opcode.disp_high << 8) | opcode.disp_low));
                            break;
                        }
                        case 0b00000010: { /* 16 bit displacement */
                            read_byte = fread(&byte, 1, 1, file);
                            if (!read_byte) { return 1; }

                            opcode.disp_low = byte;

                            read_byte = fread(&byte, 1, 1, file);
                            if (!read_byte) { return 1; }

                            opcode.disp_high = byte;

                            print_mov(&opcode, (((uint16_t) opcode.disp_high << 8) | opcode.disp_low));
                            break;
                        }
                        case 0b00000011: {
                            if (!opcode.d) {
                                uint8_t tmp = opcode.reg;
                                opcode.reg = opcode.rm;
                                opcode.rm = tmp;
                            }

                            const char * reg_code = reg_rm_11[opcode.w][(opcode.reg)];
                            const char * rm_code  = reg_rm_11[opcode.w][(opcode.rm )];

                            printf("mov %s, %s\n", reg_code, rm_code);

                            break;
                        }

                        default: { return 1; }
                    }

                    break;
                }
                case imm_to_rm: {
                    break;
                }
                case imm_to_r: {
                    opcode.w = (byte >> 3 & 0b00000001);
                    opcode.reg = (byte & 0b00000111);
                    const char *reg = reg_rm_11[opcode.w][opcode.reg];

                    if (opcode.w) {
                        size_t read_byte = fread(&byte, 1, 1, file);
                        if (!read_byte) { return 1; }
                        opcode.data_low = byte;

                        read_byte = fread(&byte, 1, 1, file);
                        if (!read_byte) { return 1; }
                        opcode.data_high = byte;

                        printf("mov %s, %d\n", reg, (((uint16_t) opcode.data_high << 8) | opcode.data_low));
                    } else {
                        size_t read_byte = fread(&byte, 1, 1, file);
                        if (!read_byte) { return 1; }
                        opcode.data_low = byte;

                        printf("mov %s, %d\n", reg, opcode.data_low);
                    }
                    break;
                }
                case mem_to_acc: {
                    opcode.w = (byte & 0b00000001);
                    if (opcode.w) {
                        size_t read_byte = fread(&byte, 1, 1, file);
                        if (!read_byte) { return 1; }
                        opcode.data_low = byte;

                        read_byte = fread(&byte, 1, 1, file);
                        if (!read_byte) { return 1; }
                        opcode.data_high = byte;

                        printf("mov ax, [%d]\n", (((uint16_t) opcode.data_high << 8) | opcode.data_low));
                    } else {
                        size_t read_byte = fread(&byte, 1, 1, file);
                        if (!read_byte) { return 1; }
                        opcode.data_low = byte;

                        printf("mov ax, [%d]\n", opcode.data_low);
                    }
                    
                    break;
                }
                case acc_to_mem: {
                    opcode.w = (byte & 0b00000001);
                    if (opcode.w) {
                        size_t read_byte = fread(&byte, 1, 1, file);
                        if (!read_byte) { return 1; }
                        opcode.data_low = byte;

                        read_byte = fread(&byte, 1, 1, file);
                        if (!read_byte) { return 1; }
                        opcode.data_high = byte;

                        printf("mov [%d], ax\n", (((uint16_t) opcode.data_high << 8) | opcode.data_low));
                    } else {
                        size_t read_byte = fread(&byte, 1, 1, file);
                        if (!read_byte) { return 1; }
                        opcode.data_low = byte;

                        printf("mov [%d], ax\n", opcode.data_low);
                    }
                    break;
                }
                case rm_to_seg: { break; }
                case seg_to_rm: { break; }
                default: {
                    fprintf(stderr, "[ERROR] Not supported MOV opcode mode: %u\n", opcode.mode);
                    return 1;
                }
            }
        }
    }

    fclose(file);

    return 0;
}

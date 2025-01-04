#include <stdio.h>

int main() {
    printf("Hello world!\n");

    /*
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

    return 0;
}
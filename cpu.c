#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

#define BASE_ADDR 0x200
#define DIGIT_SPRITES_ADDR 0x100

static uint8_t digit_sprites[] =
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

// n = 0 is the leftmost nibble!
static uint8_t opcode2nib(uint16_t opcode, int n)
{
    assert(n>=0 && n<=3);
    return (uint8_t)((opcode >> 4 * (3-n)) & 0xf);
}

// nib0 is the leftmost nibble!
static uint16_t nibs2addr(uint8_t nib0, uint8_t nib1, uint8_t nib2, uint8_t nib3)
{
    assert((nib0 & 0xf0) == 0);
    assert((nib1 & 0xf0) == 0);
    assert((nib2 & 0xf0) == 0);
    assert((nib3 & 0xf0) == 0);
    return (nib0 << 4*3) | (nib1 << 4*2) | (nib2 << 4) | nib3;
}

static uint16_t bytes2opcode(uint8_t high, uint8_t low)
{
    uint16_t tmp = high;
    tmp = tmp << 8;
    return (uint16_t)(tmp | low);
}

// nib0 is the leftmost nibble!
static uint8_t nibs2byte(uint8_t nib0, uint8_t nib1)
{
    assert((nib0 & 0xf0) == 0);
    assert((nib1 & 0xf0) == 0);
    return (nib0<<4) | nib1;
}

static void write_with_carry(struct chip8 *cpu, uint8_t dest, uint8_t val, uint8_t carry)
{
    cpu->v[dest] = val;
    cpu->v[0xf] = carry;
}

//0nnn - SYS addr
//Jump to a machine code routine at nnn.
static void instr_0nnn(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //This instruction is only used on the old computers on which Chip-8 was originally implemented. It is ignored by modern interpreters.
    //printf("0nnn - SYS addr skipped\n");
}


//00E0 - CLS
//Clear the display.
static void instr_00e0(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //printf("00E0 - CLS\n");
    memset(cpu->disp, 0, 8*32);
}

//00EE - RET
//Return from a subroutine.
static void instr_00ee(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The interpreter sets the program counter to the address at the top of the stack, then subtracts 1 from the stack pointer.
    //printf("00EE - RET\n");
    assert(cpu->sp > 0 && cpu->sp <= 0xf); 
    cpu->pc = cpu->stack[cpu->sp-1];
    cpu->sp--;
}

//1nnn - JP addr
//Jump to location nnn.
static void instr_1nnn(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //printf("1nnn - JP addr\n");
    uint16_t addr = nibs2addr(0, nib0, nib1, nib2);
    cpu->pc = addr;
}

//2nnn - CALL addr
//Call subroutine at nnn.
static void instr_2nnn(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The interpreter increments the stack pointer, then puts the current PC on the top of the stack. The PC is then set to nnn.
    //printf("2nnn - CALL addr\n");
    assert(cpu->sp >= 0 && cpu->sp < 0xf);
    cpu->sp++;
    cpu->stack[cpu->sp-1] = cpu->pc;
    uint16_t addr = nibs2addr(0, nib0, nib1, nib2);
    cpu->pc = addr;
}

//3xkk - SE Vx, byte
//Skip next instruction if Vx = kk.
static void instr_3xkk(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The interpreter compares register Vx to kk, and if they are equal, increments the program counter by 2.
    //printf("3xkk - SE Vx, byte\n");
    uint8_t kk = nibs2byte(nib1, nib2);
    if(cpu->v[nib0] == kk)
        cpu->pc += 2;
}

//4xkk - SNE Vx, byte
//Skip next instruction if Vx != kk.
static void instr_4xkk(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The interpreter compares register Vx to kk, and if they are not equal, increments the program counter by 2.
    //printf("4xkk - SNE Vx, byte\n");
    uint8_t kk = nibs2byte(nib1, nib2);
    if(cpu->v[nib0] != kk)
        cpu->pc += 2;
}


//5xy0 - SE Vx, Vy
//Skip next instruction if Vx = Vy.
static void instr_5xy0(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The interpreter compares register Vx to register Vy, and if they are equal, increments the program counter by 2.
    //printf("5xy0 - SE Vx, Vy\n");
    if(cpu->v[nib0] == cpu->v[nib1])
        cpu->pc += 2;
}

//6xkk - LD Vx, byte
//Set Vx = kk.
static void instr_6xkk(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The interpreter puts the value kk into register Vx.
    //printf("6xkk - LD Vx, byte\n");
    uint8_t kk = nibs2byte(nib1, nib2);
    cpu->v[nib0] = kk;
}

//7xkk - ADD Vx, byte
//Set Vx = Vx + kk.
static void instr_7xkk(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //Adds the value kk to the value of register Vx, then stores the result in Vx.
    //printf("7xkk - ADD Vx, byte\n");
    uint8_t kk = nibs2byte(nib1, nib2);
    uint8_t result = cpu->v[nib0] + kk;
    cpu->v[nib0] = result;
}

//8xy0 - LD Vx, Vy
//Set Vx = Vy.
static void instr_8xy0(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //Stores the value of register Vy in register Vx.
    //printf("8xy0 - LD Vx, Vy\n");
    cpu->v[nib0] = cpu->v[nib1];
}

//8xy1 - OR Vx, Vy
//Set Vx = Vx OR Vy.
static void instr_8xy1(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx. A bitwise OR compares the corrseponding bits from two values, and if either bit is 1, then the same bit in the result is also 1. Otherwise, it is 0.
    //printf("8xy1 - OR Vx, Vy\n");
    cpu->v[nib0] |= cpu->v[nib1];
}

//8xy2 - AND Vx, Vy
//Set Vx = Vx AND Vy.
static void instr_8xy2(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx. A bitwise AND compares the corrseponding bits from two values, and if both bits are 1, then the same bit in the result is also 1. Otherwise, it is 0.
    //printf("8xy2 - AND Vx, Vy\n");
    cpu->v[nib0] &= cpu->v[nib1];
}

//8xy3 - XOR Vx, Vy
//Set Vx = Vx XOR Vy.
static void instr_8xy3(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //Performs a bitwise exclusive OR on the values of Vx and Vy, then stores the result in Vx. An exclusive OR compares the corrseponding bits from two values, and if the bits are not both the same, then the corresponding bit in the result is set to 1. Otherwise, it is 0.
    //printf("8xy3 - XOR Vx, Vy\n");
    cpu->v[nib0] ^= cpu->v[nib1];
}

//8xy4 - ADD Vx, Vy
//Set Vx = Vx + Vy, set VF = carry.
static void instr_8xy4(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The values of Vx and Vy are added together. If the result is greater than 8 bits (i.e., > 255) VF is set to 1, otherwise 0. Only the lowest 8 bits of the result are kept, and stored in Vx.
    //printf("8xy4 - ADD Vx, Vy\n");
    uint16_t tmp = cpu->v[nib0];
    tmp += cpu->v[nib1];
    cpu->v[nib0] = tmp & 0xff;
    cpu->v[0xf] = (tmp & 0xff00) != 0;
}

//8xy5 - SUB Vx, Vy
//Set Vx = Vx - Vy, set VF = NOT borrow.
static void instr_8xy5(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.
    //printf("8xy5 - SUB Vx, Vy\n");
    write_with_carry(
        cpu,
        nib0,
        cpu->v[nib0] - cpu->v[nib1],
        cpu->v[nib0] >= cpu->v[nib1]);
}

//8xy6 - SHR Vx {, Vy}
//Set Vx = Vx SHR 1.
static void instr_8xy6(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
    //printf("8xy6 - SHR Vx {, Vy}\n");
    write_with_carry(
        cpu,
        nib0,
        cpu->v[nib0] >> 1,
        cpu->v[nib0] & 0x1);
}

//8xy7 - SUBN Vx, Vy
//Set Vx = Vy - Vx, set VF = NOT borrow.
static void instr_8xy7(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
    //printf("8xy7 - SUBN Vx, Vy\n");
    write_with_carry(
        cpu,
        nib0,
        cpu->v[nib1] - cpu->v[nib0],
        cpu->v[nib1] >= cpu->v[nib0]);
}

//8xyE - SHL Vx {, Vy}
//Set Vx = Vx SHL 1.
static void instr_8xye(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
    //printf("8xyE - SHL Vx {, Vy}\n");
    write_with_carry(
        cpu,
        nib0,
        cpu->v[nib1] << 1,
        (cpu->v[nib1] >> 7) & 0x1);
}

//9xy0 - SNE Vx, Vy
//Skip next instruction if Vx != Vy.
static void instr_9xy0(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.
    //printf("9xy0 - SNE Vx, Vy\n");
    if(cpu->v[nib0] != cpu->v[nib1])
        cpu->pc += 2;
}

//Annn - LD I, addr
//Set I = nnn.
static void instr_annn(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The value of register I is set to nnn.
    //printf("Annn - LD I, addr\n");
    uint16_t addr = nibs2addr(0, nib0, nib1, nib2);
    cpu->i = addr;
}

//Bnnn - JP V0, addr
//Jump to location nnn + V0.
static void instr_bnnn(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The program counter is set to nnn plus the value of V0.
    //printf("Bnnn - JP V0, addr\n");
    uint16_t addr = nibs2addr(0, nib0, nib1, nib2) + cpu->v[0];
    cpu->pc = addr;
}

//Cxkk - RND Vx, byte
//Set Vx = random byte AND kk.
static void instr_cxkk(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The interpreter generates a random number from 0 to 255, which is then ANDed with the value kk. The results are stored in Vx. See instruction 8xy2 for more information on AND.
    //printf("Cxkk - RND Vx, byte\n");
    uint8_t mask = nibs2byte(nib1, nib2);
    cpu->v[nib0] = (rand() % 0xff) & mask;
}


// XOR the bit in memory that represents
// pixel x, y with 1
// Return true if the pixel was previously already 1
static int xor_pixel(struct chip8 *cpu, int x, int y)
{
    // Screen is 64 x 32 px, each px is a bit
    x = x % 64;
    y = y % 32;

    // Find byte to manipulate the target pixel
    int target_byte = (y * 8) + (x / 8);

    // Bitmask to manipulate pixel
    uint8_t value_mask = (uint8_t) 128 >> (x % 8);

    // Do it
    cpu->disp[target_byte] ^= value_mask;

    // Check if there was a collision
    return cpu->disp[target_byte] & value_mask;
}

//Dxyn - DRW Vx, Vy, nibble
//Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
static void instr_dxyn(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //printf("Dxyn - DRW Vx, Vy, nibble\n");

    uint8_t start_x = cpu->v[nib0];
    uint8_t start_y = cpu->v[nib1];

    cpu->v[0xf] = 0;

    for(int row=0; row<nib2; row++)
    {
        uint8_t sprite_row = cpu->mem[cpu->i+row];
        for(int px=0; px<8; px++)
        {
            //printf("Checking if sprite_row=0x%02X px=%d is set (&0x%02X)\n", sprite_row, px, (128 >> px));

            if(sprite_row & (128 >> px)) // just bit[px] of the current row
            {
                //printf("XORing pixel x=%d, y=%d\n", start_x + px, start_y + row);
                int x = start_x + px;
                int y = start_y + row;
                int result = xor_pixel(cpu, x, y);
                if(result == 0)
                {
                    cpu->v[0xf] = 1;
                }
            }
        }
    }
}

//Ex9E - SKP Vx
//Skip next instruction if key with the value of Vx is pressed.
static void instr_ex9e(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //Checks the keyboard, and if the key corresponding to the value of Vx is currently in the down position, PC is increased by 2.
    //printf("Ex9E - SKP Vx\n");
    uint16_t mask = 0x1 << cpu->v[nib0];
    if((cpu->keys & mask) != 0)
    {
        cpu->pc += 2;
    }
}

//ExA1 - SKNP Vx
//Skip next instruction if key with the value of Vx is not pressed.
static void instr_exa1(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //Checks the keyboard, and if the key corresponding to the value of Vx is currently in the up position, PC is increased by 2.
    //printf("ExA1 - SKNP Vx\n");
    uint16_t mask = 0x1 << cpu->v[nib0];
    if((cpu->keys & mask) == 0)
    {
        cpu->pc += 2;
    }
}

//Fx07 - LD Vx, DT
//Set Vx = delay timer value.
static void instr_fx07(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The value of DT is placed into Vx.
    //printf("Fx07 - LD Vx, DT");
    cpu->v[nib0] = cpu->dt;
}

//Fx0A - LD Vx, K
//Wait for a key press, store the value of the key in Vx.
static void instr_fx0a(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //All execution stops until a key is pressed, then the value of that key is stored in Vx.
    //printf("Fx0A - LD Vx, K\n");
    cpu->wait_key = 1;
    cpu->key_vx = nib0;
}

//Fx15 - LD DT, Vx
//Set delay timer = Vx.
static void instr_fx15(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //DT is set equal to the value of Vx.
    //printf("Fx15 - LD DT, Vx\n");
    cpu->dt = cpu->v[nib0];
}

//Fx18 - LD ST, Vx
//Set sound timer = Vx.
static void instr_fx18(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //ST is set equal to the value of Vx.
    //printf("Fx18 - LD ST, Vx\n");
    cpu->st = cpu->v[nib0];
}

//Fx1E - ADD I, Vx
//Set I = I + Vx.
static void instr_fx1e(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The values of I and Vx are added, and the results are stored in I.
    //printf("Fx1E - ADD I, Vx\n");
    cpu->i += cpu->v[nib0];
}

//Fx29 - LD F, Vx
//Set I = location of sprite for digit Vx.
static void instr_fx29(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The value of I is set to the location for the hexadecimal sprite corresponding to the value of Vx. See section 2.4, Display, for more information on the Chip-8 hexadecimal font.
    //printf("Fx29 - LD F, Vx");
    cpu->i = DIGIT_SPRITES_ADDR + (cpu->v[nib0] * 5);
}

//Fx33 - LD B, Vx
//Store BCD representation of Vx in memory locations I, I+1, and I+2.
static void instr_fx33(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The interpreter takes the decimal value of Vx, and places the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
    //printf("Fx33 - LD B, Vx\n");
    cpu->mem[cpu->i] = (cpu->v[nib0] / 100) % 10;
    cpu->mem[cpu->i+1] = (cpu->v[nib0] / 10) % 10;
    cpu->mem[cpu->i+2] = cpu->v[nib0] % 10;
}

//Fx55 - LD [I], Vx
//Store registers V0 through Vx in memory starting at location I.
static void instr_fx55(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The interpreter copies the values of registers V0 through Vx into memory, starting at the address in I.
    //printf("Fx55 - LD [I], Vx\n");
    for(int i = 0; i <= nib0; i++)
    {
        cpu->mem[cpu->i+i] = cpu->v[i];
    }
}

//Fx65 - LD Vx, [I]
//Read registers V0 through Vx from memory starting at location I.
static void instr_fx65(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    //The interpreter reads values from memory starting at location I into registers V0 through Vx.
    //printf("Fx65 - LD Vx, [I]\n");
    for(int i = 0; i <= nib0; i++)
    {
        cpu->v[i] = cpu->mem[cpu->i+i];
    }
}

static void dummy_instr(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib2)
{
    printf("Instruction not implemented!\n");
    exit(0);
}

typedef void (*instrp_t)(struct chip8 *cpu, uint8_t nib0, uint8_t nib1, uint8_t nib3);

static uint16_t fetch(struct chip8 *cpu)
{
    //printf("Fetching @PC=0x%04X\n", cpu->pc);
    //assert(cpu->pc % 2 == 0);
    uint16_t result = bytes2opcode(cpu->mem[cpu->pc], cpu->mem[cpu->pc+1]);
    cpu->pc += 2;
    return result;
}

// TODO use binary AND for filtered comparison
static instrp_t decode(uint16_t opcode)
{
    //printf("Decoding opcode=0x%04X\n", opcode);
    uint8_t nib0 = opcode2nib(opcode, 0);
    uint8_t nib1 = opcode2nib(opcode, 1);
    uint8_t nib2 = opcode2nib(opcode, 2);
    uint8_t nib3 = opcode2nib(opcode, 3);
    switch(nib0)
    {
        case 0x0:
            if(nib1 == 0x0 && nib2 == 0xe && nib3 == 0x0)
                return &instr_00e0;
            else if(nib1 == 0x0 && nib2 == 0xe && nib3 == 0xe)
                return &instr_00ee;
            else
                return &instr_0nnn;
        case 0x1:
            return &instr_1nnn;
        case 0x2:
            return &instr_2nnn;
        case 0x3:
            return &instr_3xkk;
        case 0x4:
            return &instr_4xkk;
        case 0x5:
            if(nib3 == 0x0)
                return &instr_5xy0;
            break;
        case 0x6:
            return &instr_6xkk;
        case 0x7:
            return &instr_7xkk;
        case 0x8:
            if(nib3 == 0x0)
                return &instr_8xy0;
            else if(nib3 == 0x1)
                return &instr_8xy1;
            else if(nib3 == 0x2)
                return &instr_8xy2;
            else if(nib3 == 0x3)
                return &instr_8xy3;
            else if(nib3 == 0x4)
                return &instr_8xy4;
            else if(nib3 == 0x5)
                return &instr_8xy5;
            else if(nib3 == 0x6)
                return &instr_8xy6;
            else if(nib3 == 0x7)
                return &instr_8xy7;
            else if(nib3 == 0xe)
                return &instr_8xye;
            break;
        case 0x9:
            if(nib3 == 0x0)
                return &instr_9xy0;
            break;
        case 0xa:
            return &instr_annn;
        case 0xb:
            return &instr_bnnn;
        case 0xc:
            return &instr_cxkk;
        case 0xd:
            return &instr_dxyn;
        case 0xe:
            if(nib2 == 0x9 && nib3 == 0xe)
                return &instr_ex9e;
            else if(nib2 == 0xa && nib3 == 0x1)
                return &instr_exa1;
            break;
        case 0xf:
            if(nib2 == 0x0 && nib3 == 0x7)
                return &instr_fx07;
            else if (nib2 == 0x0 && nib3 == 0xa)
                return &instr_fx0a;
            else if (nib2 == 0x1 && nib3 == 0x5)
                return &instr_fx15;
            else if (nib2 == 0x1 && nib3 == 0x8)
                return &instr_fx18;
            else if (nib2 == 0x1 && nib3 == 0xe)
                return &instr_fx1e;
            else if (nib2 == 0x2 && nib3 == 0x9)
                return &instr_fx29;
            else if (nib2 == 0x3 && nib3 == 0x3)
                return &instr_fx33;
            else if (nib2 == 0x5 && nib3 == 0x5)
                return &instr_fx55;
            else if (nib2 == 0x6 && nib3 == 0x5)
                return &instr_fx65;
            break;
        default:
            return &dummy_instr;
    }
    return &dummy_instr;
}

static void execute(struct chip8 *cpu, instrp_t instr, uint16_t opcode)
{
    uint8_t nib1 = opcode2nib(opcode, 1);
    uint8_t nib2 = opcode2nib(opcode, 2);
    uint8_t nib3 = opcode2nib(opcode, 3);
    (*instr)(cpu, nib1, nib2, nib3);
}

void cpu_cycle(struct chip8 *cpu)
{
    if(cpu->wait_key) return;
    uint16_t opcode = fetch(cpu);
    instrp_t instr = decode(opcode);
    execute(cpu, instr, opcode);
}

void cpu_tick60hz(struct chip8 *cpu)
{
    if(cpu->dt > 0) cpu->dt--;
    if(cpu->st > 0) cpu->st--;
}

void cpu_reset(struct chip8 *cpu)
{
    memset(cpu->v, 0, 0xf);
    memset(cpu->stack, 0, 0xf * sizeof(uint16_t));
    memset(cpu->disp, 0, 8*32);
    memcpy(cpu->mem + DIGIT_SPRITES_ADDR, &digit_sprites, sizeof(digit_sprites));
    cpu->i = 0;
    cpu->dt = 0;
    cpu->st = 0;
    cpu->sp = 0;
    cpu->pc = BASE_ADDR;
    cpu->keys = 0;
    cpu->wait_key = 0;
    cpu->key_vx = 0;
}

void cpu_init(struct chip8 *cpu)
{
    memset(cpu->mem, 0, 0xfff);
    cpu_reset(cpu);
    srand(1234);
}

void cpu_load_rom(struct chip8 *cpu, const char *path)
{
    FILE *fs = fopen(path, "rb");
    fseek(fs, 0, SEEK_END);
    size_t fsize = ftell(fs);
    assert(fsize <= 0xfff-BASE_ADDR);
    rewind(fs);
    size_t read = 0;
    while(read < fsize)
    {
        read += fread(cpu->mem + BASE_ADDR + read, 1, fsize, fs);
    }
    fclose(fs);
}

int cpu_get_pixel(struct chip8 *cpu, int x, int y)
{
    x = x % 64;
    y = y % 32;

    int target_byte = (y * 8) + (x / 8);

    uint8_t value_mask = (uint8_t) 128 >> (x % 8);

    int result = cpu->disp[target_byte] & value_mask;

    return result;
}

void cpu_set_key_state(struct chip8 *cpu, uint8_t key, uint8_t state)
{
    assert(key >= 0x0 && key <= 0xf);

    if(state)
    {
        cpu->keys |= (1 << key);
        if(cpu->wait_key)
        {
            cpu->v[cpu->key_vx] = key;
            cpu->wait_key = 0;
        }
        printf("key pressed = %i, keys = %i\n", key, cpu->keys);
        //exit(0);
    }
    else
    {
        cpu->keys &= ~(1 << key);
        //printf("key released = %i, keys = %i\n", key, cpu->keys);
    }
}

void cpu_dump_state(struct chip8 *cpu)
{
}

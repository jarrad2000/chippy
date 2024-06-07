#ifndef CHIPPY_CPU_H
#define CHIPPY_CPU_H

#include <stdint.h>

// TODO use union to overlap registers etc. with  memory
struct chip8
{
    uint8_t mem[4096]; // memory
    uint8_t v[16]; // general purpose registers
    uint16_t i; // address register
    uint8_t dt; // delay timer
    uint8_t st; // sound timer
    uint8_t sp; // stack pointer
    uint16_t stack[16]; // stack
    uint16_t pc; // program counter
    uint16_t keys; // keypad keys
    uint8_t disp[8*32]; // 64x32 bit
    uint8_t wait_key; // waiting for key event
    uint8_t key_vx; // v index to store pressed key
};

void cpu_cycle(struct chip8 *cpu);

void cpu_tick60hz(struct chip8 *cpu);

void cpu_reset(struct chip8 *cpu);

void cpu_init(struct chip8 *cpu);

void cpu_load_rom(struct chip8 *cpu, const char *path);

int cpu_get_pixel(struct chip8 *cpu, int x, int y);

void cpu_set_key_state(struct chip8 *cpu, uint8_t key, uint8_t state);

void cpu_dump_state(struct chip8 *cpu);

#endif

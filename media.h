#ifndef CHIPPY_MEDIA_H
#define CHIPPY_MEDIA_H

#include <stdint.h>
#ifdef _WIN32
#include <SDL.h>
#include <SDL_audio.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#endif

#define TEXTURE_WIDTH 64
#define TEXTURE_HEIGHT 32

struct sdl_audio
{
    float last_tone;
    SDL_AudioSpec audio_spec;
    SDL_AudioDeviceID audio_dev;
};

struct sdl_graphics
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    unsigned int pixels[TEXTURE_WIDTH * TEXTURE_HEIGHT * 4];
};

struct chip8_media
{
    struct sdl_audio audio;
    struct sdl_graphics graphics;
};

int media_init(struct chip8_media *media);

void media_close(struct chip8_media *media);

void media_set_pixel(struct chip8_media *media, int x, int y, int active);

void media_set_pixel_dbg(struct chip8_media *media, int x, int y, int active);

void media_set_buzzer(struct chip8_media *media, int active);

void media_render(struct chip8_media *media);

int media_poll_exit_requested(struct chip8_media *media);

int media_poll_key_down(struct chip8_media *media, uint8_t key);

uint32_t media_ms_elapsed(struct chip8_media *media);

void media_ms_delay(struct chip8_media *media, uint32_t ms);

#endif
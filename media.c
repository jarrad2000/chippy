#include <stdio.h>
#include <math.h>

#ifdef _WIN32
#include <SDL.h>
#include <SDL_audio.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#endif

#include "media.h"

#define SPEAKER_FREQ 440
#define SAMPLING_FREQ 44100

#define PI2 6.283185307179586

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

static const float TONE_INC = (float)(PI2 *  SPEAKER_FREQ / SAMPLING_FREQ);

static void audio_callback(void* user_data, uint8_t* stream, int len)
{
    float* last_tone = (float*)user_data;

    for (int i = 0; i < len; i++)
    {
        stream[i] = (uint8_t) (sinf(*last_tone) + 127);
        (*last_tone) += TONE_INC;
    }
}

static int init_audio(struct sdl_audio *sa)
{
    sa->last_tone = 0;

    sa->audio_spec.freq = SAMPLING_FREQ; // number of samples per second
    sa->audio_spec.format = AUDIO_U8; // sample type (here: unsigned 8 bit)
    sa->audio_spec.channels = 1; // only one channel
    sa->audio_spec.samples = 4096; // buffer-size
    sa->audio_spec.callback = audio_callback; // function SDL calls periodically to refill the buffer
    sa->audio_spec.userdata = &sa->last_tone; // counter, keeping track of current sample number
    SDL_AudioSpec have;

    sa->audio_dev = SDL_OpenAudioDevice(NULL, 0, &sa->audio_spec, &have, 0);

    if (sa->audio_dev == 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
        return 1;
    }

    if (sa->audio_spec.format != have.format)
    {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired audio spec");
        return 1;
    }
    return 0;
}

static void close_sound(struct sdl_audio *sa)
{
    if(sa->audio_dev != 0)
        SDL_CloseAudioDevice(sa->audio_dev);
    sa->audio_dev = 0;
}

static int init_graphics(struct sdl_graphics *sg)
{
    sg->window = SDL_CreateWindow("Chippy",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    if(sg->window == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create window: %s", SDL_GetError());
        return 1;
    }

    SDL_SetWindowMinimumSize(sg->window, TEXTURE_WIDTH, TEXTURE_HEIGHT);

    sg->renderer = SDL_CreateRenderer(sg->window, -1, SDL_RENDERER_PRESENTVSYNC);

    if(sg->renderer == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create renderer: %s", SDL_GetError());
        return 1;
    }

    if(SDL_RenderSetLogicalSize(sg->renderer, TEXTURE_WIDTH, TEXTURE_HEIGHT) != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to set logical size: %s", SDL_GetError());
        return 1;
    }

    if(SDL_RenderSetIntegerScale(sg->renderer, 1) != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to set scaling: %s", SDL_GetError());
        return 1;
    }

    sg->texture = SDL_CreateTexture(sg->renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        TEXTURE_WIDTH, TEXTURE_HEIGHT);

    if(sg->texture == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create texture: %s", SDL_GetError());
        return 1;
    }

    return 0;
}

static void close_graphics(struct sdl_graphics *sg)
{
    if (sg->texture != NULL)
        SDL_DestroyTexture(sg->texture);
    sg->texture = NULL;

    if (sg->renderer != NULL)
        SDL_DestroyRenderer(sg->renderer);
    sg->renderer = NULL;

    if (sg->window != NULL)
        SDL_DestroyWindow(sg->window);
    sg->window = NULL;
}

int media_init(struct chip8_media *media)
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0) 
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Initializing media");

    if (init_audio(&media->audio) != 0)
        return 1;

    if (init_graphics(&media->graphics) != 0)
        return 1;
    return 0;
}

void media_close(struct chip8_media *media)
{
    close_sound(&media->audio);
    close_graphics(&media->graphics);

    SDL_Quit();
}

void media_set_pixel(struct chip8_media *media, int x, int y, int active)
{
    if(active)
        media->graphics.pixels[x + y * TEXTURE_WIDTH] = 0xffffffff;
    else
        media->graphics.pixels[x + y * TEXTURE_WIDTH] = 0x333333ff;
}

void media_set_pixel_dbg(struct chip8_media *media, int x, int y, int active)
{
    if(active)
        media->graphics.pixels[x + y * TEXTURE_WIDTH] = 0x00ff002f;
    else
        media->graphics.pixels[x + y * TEXTURE_WIDTH] = 0xff00002f;
}

void media_set_buzzer(struct chip8_media *media, int active)
{
    if (active)
        SDL_PauseAudioDevice(media->audio.audio_dev, 0);
    else
        SDL_PauseAudioDevice(media->audio.audio_dev, 1);
}

void media_render(struct chip8_media *media)
{
    SDL_RenderClear(media->graphics.renderer);
    SDL_UpdateTexture(media->graphics.texture, NULL, media->graphics.pixels, TEXTURE_WIDTH * 4);
    SDL_RenderCopy(media->graphics.renderer, media->graphics.texture, NULL, NULL);
    SDL_RenderPresent(media->graphics.renderer);
}

int media_poll_exit_requested(struct chip8_media *media)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev))
    {
        if (ev.type == SDL_QUIT)
            return 1;
    }
    return 0;
}

const int chip8_to_sdl_keymap[] =
{
    SDL_SCANCODE_X, // 0
    SDL_SCANCODE_1, // 1
    SDL_SCANCODE_2, // 2
    SDL_SCANCODE_3, // 3
    SDL_SCANCODE_Q, // 4
    SDL_SCANCODE_W, // 5
    SDL_SCANCODE_E, // 6
    SDL_SCANCODE_A, // 7
    SDL_SCANCODE_S, // 8
    SDL_SCANCODE_D, // 9
    SDL_SCANCODE_Y, // A
    SDL_SCANCODE_C, // B
    SDL_SCANCODE_4, // C
    SDL_SCANCODE_R, // D
    SDL_SCANCODE_F, // E
    SDL_SCANCODE_V  // F
};

int media_poll_key_down(struct chip8_media *media, uint8_t chip8_key)
{
    const Uint8* sdl_keystate;

    if (chip8_key < 0 || chip8_key > 15) return 0;
    SDL_PumpEvents(); // required?
    sdl_keystate = SDL_GetKeyboardState(NULL);
    int sdl_key = chip8_to_sdl_keymap[chip8_key];

    return sdl_keystate[sdl_key];
}

uint32_t media_ms_elapsed(struct chip8_media *media)
{
    return SDL_GetTicks();
}

void media_ms_delay(struct chip8_media *media, uint32_t ms)
{
    SDL_Delay(ms);
}

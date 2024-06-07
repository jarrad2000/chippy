#include <stdio.h>
#include "cpu.h"
#include "media.h"

struct chip8_media media;
struct chip8 cpu;

#define NO_CYCLES (500 / 60)

int main(int argc, char* argv[])
{
    // cpu initialization

    cpu_init(&cpu);
    printf("argc=%i\n", argc);
    for(int i=0; i<argc; i++)
    {
        printf("argv[%i]=", i);
        printf("%s", argv[i]);
        printf("\n");
    }

    if(argc > 1)
        cpu_load_rom(&cpu, argv[1]);

    // media initialization

    if (media_init(&media) != 0)
        exit(0);

    // main loop

    uint32_t ms_start = 0;// = media_ms_elapsed(&media);

    while (!media_poll_exit_requested(&media))
    {
        ms_start = media_ms_elapsed(&media);

        for(int i=0; i<NO_CYCLES;i++)
            cpu_cycle(&cpu);
        cpu_tick60hz(&cpu);
        media_set_buzzer(&media, cpu.st > 0);

        for(uint8_t i=0; i<0x10; i++)
        {
            int key_down = media_poll_key_down(&media, i);
            cpu_set_key_state(&cpu, i, key_down);
        }

        for (int y = 0; y < TEXTURE_HEIGHT; ++y)
        {
            for (int x = 0; x < TEXTURE_WIDTH; ++x)
            {
                media_set_pixel(&media, x, y,
                    cpu_get_pixel(&cpu, x, y));
            }
        }

        media_render(&media);

        uint32_t ms_elapsed = media_ms_elapsed(&media) - ms_start;
        if (ms_elapsed < 16)
            media_ms_delay(&media, 16 - ms_elapsed);
        else
            media_ms_delay(&media, 1);
    }

    // shutdown

    media_close(&media);

    return 0;
}

# chippy
Chip8 emulator written in C99, built with zig, SDL2 is used for sound &amp; graphics 

Wrote it to learn about emulators, the code is often very inefficient since my primary focus was to get it working at all.

I tried to keep SDL and CPU emulation completely separate, they are glued together in the main loop. In addition I tried to stay away from the memory allocator. This constraint has no practical purpose, it was also meant as a learning experience to see how far I could get without using it.

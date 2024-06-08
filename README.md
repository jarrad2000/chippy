# chippy
Chip8 emulator written in C, built with zig, SDL2 is used for sound &amp; graphics 

![chippy-brix](https://github.com/jarrad2000/chippy/assets/1764091/ca5c8823-17c8-4ca4-b563-ebd167c61536)

Wrote it to refresh my C skills and learn about emulators and SDL. The code is often very inefficient since my primary focus was to get it working at all.

I tried to keep the SDL part and CPU emulation completely separate, they are glued together in the main loop. In addition I tried to stay away from the memory allocator. This constraint has no practical purpose, it was also meant as a learning experience to see how far I could get without using it. I used zig to build it, also as a learning experience.

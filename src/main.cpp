#include <stdio.h>
#include <SDL2/SDL.h>

#include "emu/cpu/8086_cpu.h"

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;

    // Read BIOS.ROM into memory
    FILE* bios = fopen("BIOS.ROM", "rb");
    if (bios == NULL) {
        printf("Failed to open BIOS.ROM\n");
        return 1;
    }

    fseek(bios, 0, SEEK_END);
    size_t bios_size = ftell(bios);
    fseek(bios, 0, SEEK_SET);
    for (size_t i = 0; i < bios_size; i++) {
        mem.write(i, fgetc(bios));
    }

    execute_z86();

    // printf("%s", cpu.GetRegisterState().c_str());

    // if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    //     printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    // } else {
    //     window = SDL_CreateWindow("PC98", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
    //     if (window == NULL) {
    //         printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    //     } else {
    //         screenSurface = SDL_GetWindowSurface(window);
    //         SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));
    //         SDL_UpdateWindowSurface(window);

    //         SDL_Event event; 
    //         bool quit = false; 
    //         while(quit == false){
    //             while(SDL_PollEvent(&event)){
    //                 if(event.type == SDL_QUIT) quit = true;
    //             }
    //         }
    //     }
    // }

    // SDL_DestroyWindow(window);
    // SDL_Quit();

    return 0;
}
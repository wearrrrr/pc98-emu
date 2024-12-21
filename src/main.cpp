#include <stdio.h>
#include <SDL2/SDL.h>

#include "emu/cpu.h"

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;

    CPU cpu = CPU();

    uint8_t program[] = {
        0xB0, 0x12,
        0xB4, 0x34,
        0xB3, 0x56,
        0xB8, 0x9A, 0x78,
        0xF4
    };

    cpu.load(program, sizeof(program));
    for (int i = 0; i < sizeof(program); i++) {
        cpu.clock();
    }

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
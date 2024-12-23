#include <stdio.h>
#include <SDL2/SDL.h>

#include "emu/cpu/8086_cpu.h"

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;

    // Read BIOS.ROM into memory
    FILE* bios = fopen("BIOS.ROM", "rb");
    
    if (bios == NULL) {
        printf("BIOS.ROM not found\n");
        return 1;
    }

    fseek(bios, 0, SEEK_END);
    size_t bios_size = ftell(bios);
    fseek(bios, 0, SEEK_SET);

    uint8_t* bios_data = (uint8_t*)malloc(bios_size);
    fread(bios_data, 1, bios_size, bios);

    // Print the start of the bios data as hex
    for (int i = 0; i < 32; i++) {
        printf("%02X ", bios_data[i]);
    }
    printf("\n");

    fclose(bios);

    for (int i = 0; i < bios_size; i++) {
        mem.write(0xF0000 + i, bios_data[i]);
    }


    // uint8_t program[] = {
    //     0xEB, 0x22
    // };

    // mem.write(0xFFFF0, program);

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
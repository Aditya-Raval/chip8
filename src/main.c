#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "SDL.h"

typedef struct{
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_t;

typedef struct{
    uint32_t window_height;
    uint32_t window_width;
    uint32_t fgcolor;
    uint32_t bgcolor;
    uint32_t scale_factor;
    uint32_t insts_per_second; //CPU SPeed
} config_t;
typedef enum {
    QUIT,
    RUNNING,
    PAUSED,
} emu_state;

typedef struct {
    uint16_t opcode;
    uint16_t NNN;   //12 bit add/const
    uint8_t NN;     //8 bit const
    uint8_t N;      //4 bit const
    uint8_t X;      //4 bit register name
    uint8_t Y;     //4 bit register name
} instruction_t;
//CHIP-8 Object 
typedef struct {
    emu_state state;
    uint16_t PC; //program counter
    uint8_t V[16]; //V registers
    uint16_t I;     //ram index reg
    uint8_t ram[4096];
    uint16_t stack[16]; //subroutine stack
    uint16_t *stack_ptr; //stack pointer
    bool display[64*32]; //display {is stored in ram in original machine}
    uint8_t delay_timer;
    uint8_t display_timer; //decrements at 60hz when >0
    uint8_t sound_timer;    //decrements at 60hz and plays a sound when >0
    bool keypad[16]; //keypad   
    char *rom_name; //currently running rom
    instruction_t inst; //currently executing instruction
} chip8_t;

bool set_config_from_args(config_t *config,int argc,char **argv){
    *config = (config_t){
        .window_height = 32,
        .window_width = 64,
        .fgcolor = 0XF3E2D4FF,
        .bgcolor = 0x17313EFF,
        .scale_factor = 20,
        .insts_per_second = 500,
    };

    for(int i=0;i<argc;i++){
        (void)argv[i];
    }
    return true;    
}

bool init_sdl(sdl_t *sdl,const config_t config){
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0){
        SDL_Log("Could Not init SDL : %s",SDL_GetError());
        return false;
    }
    //sdl window init
    sdl->window = SDL_CreateWindow("CHIP-8",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,config.window_width * config.scale_factor,config.window_height * config.scale_factor,0);
    if(!sdl->window){
        SDL_Log("\nCould not create SDL window : %s",SDL_GetError());
        return false;
    }
    //sdl renderer init
    sdl->renderer = SDL_CreateRenderer(sdl->window,-1,SDL_RENDERER_ACCELERATED);
    if(!sdl->renderer){
        SDL_Log("\nCould not create SDL renderer : %s",SDL_GetError());
        return false;
    }
    return true;
}

bool init_chip8(chip8_t *chip8,char rom_name[]){

    const uint32_t entry_point = 0x200; //entry for ROM
    const uint8_t font[] = {
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

    memcpy(&chip8->ram[0],font,sizeof(font));
    chip8->rom_name = rom_name;
    FILE *rom = fopen(rom_name, "rb");
    if(!rom){
        SDL_Log("Rom file %s is invalid or does not exist \n",rom_name);
        return false;
    }
    
    fseek(rom,0,SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t max_size = sizeof chip8->ram - entry_point;    
    rewind(rom);

    if(rom_size > max_size){
        SDL_Log("Rom file %s exceed the memory limit\n",rom_name);
        return false;
    }

    if(fread(&chip8->ram[entry_point],rom_size,1,rom)!=1){
        SDL_Log("could not read rom file into memory\n");
        return false;
    }
    chip8->PC=entry_point;
    fclose(rom);
    chip8->state = RUNNING;
    chip8->stack_ptr = &chip8->stack[0]; 
    return true;
}

#ifdef DEBUG
void print_debug_info(chip8_t *chip8){
    printf("Address : 0x%04X, Opcode : 0x%04X Desc : ",chip8->PC-2,chip8->inst.opcode);
    switch((chip8->inst.opcode >> 12) & 0x0F){
        case 0x0:
            //clear screen
            if(chip8->inst.NN == 0xE0){
                printf("Clear Screen \n");
            } else if (chip8->inst.NN == 0xEE){
                //return from subroutin
                printf("Return from subroutine to address 0x%04X\n",*(chip8->stack_ptr-1));
            }
            break;
        default: printf("Unimplemented Opcode\n"); break;
    }
}
#endif
void emu_instruction(chip8_t *chip8,const config_t config){
    chip8->inst.opcode = (chip8->ram[chip8->PC] << 8) | chip8->ram[chip8->PC+1];
    chip8->PC += 2;
    
    //instruction format 
    chip8->inst.NNN = chip8->inst.opcode & 0x0FFF;
    chip8->inst.NN = chip8->inst.opcode & 0x0FF;
    chip8->inst.N = chip8->inst.opcode & 0x0F;
    chip8->inst.X = (chip8->inst.opcode >> 8) & 0x0F;
    chip8->inst.Y = (chip8->inst.opcode >> 4) & 0x0F; 

    //opcodes 
    switch((chip8->inst.opcode >> 12) & 0x0F){
        case 0x0:
            //clear screen
            if(chip8->inst.NN == 0xE0){
                memset(&chip8->display[0],false,sizeof chip8->display);
            } else if (chip8->inst.NN == 0xEE){
                //return from subroutine
                chip8->PC = *--chip8->stack_ptr;
            }
            break;
        
        case 0x01:
            //jmp to NNN
            chip8->PC = chip8->inst.NNN;
            break;
        
        case 0x02:
            //call subroutine
            *chip8->stack_ptr++ = chip8->PC;
            chip8->PC = chip8->inst.NNN;
            break;

        case 0x03:
            if(chip8->V[chip8->inst.X]==chip8->inst.NN){
                chip8->PC += 2;
            }
            break;

        case 0x04:
            if(chip8->V[chip8->inst.X]!=chip8->inst.NN){
                chip8->PC += 2;
            }
            break;
        case 0x05:
            if(chip8->inst.N!=0) break;
            if(chip8->V[chip8->inst.X]==chip8->V[chip8->inst.Y]){
                chip8->PC += 2;
            }
            break;
       
        case 0x06:
            //set VX to NN
            chip8->V[chip8->inst.X] = chip8->inst.NN;
            break;
        
        case 0x07:
            //set VX plus equals to NN 
            chip8->V[chip8->inst.X] += chip8->inst.NN;
            break;
        case 0x08:
            switch(chip8->inst.N){
                case 0:
                    chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y];
                    break;

                case 1:
                    chip8->V[chip8->inst.X] |= chip8->V[chip8->inst.Y];
                    break;
                
                case 2:
                    chip8->V[chip8->inst.X] &= chip8->V[chip8->inst.Y];
                    break;
                
                case 3:
                    chip8->V[chip8->inst.X] ^= chip8->V[chip8->inst.Y];
                    break;
                
                case 4:
                    if((uint16_t)(chip8->V[chip8->inst.X]+chip8->V[chip8->inst.Y])>255) 
                        chip8->V[0xF]=1;
                    chip8->V[chip8->inst.X] += chip8->V[chip8->inst.Y];
                    break;
                
                case 5:
                    chip8->V[0xF] = (chip8->V[chip8->inst.Y] <= chip8->V[chip8->inst.X]);
                    chip8->V[chip8->inst.X] -= chip8->V[chip8->inst.Y];
                    break;
                
                case 6:
                    chip8->V[0xF]=chip8->V[chip8->inst.X] & 1;
                    chip8->V[chip8->inst.X]>>=1;
                    break;
                
                case 7:
                    chip8->V[0xF] = (chip8->V[chip8->inst.X] <= chip8->V[chip8->inst.Y]);
                    chip8->V[chip8->inst.X] = chip8->V[chip8->inst.Y] - chip8->V[chip8->inst.X];
                    break;

                case 0xE:
                    chip8->V[0xF]=(chip8->V[chip8->inst.X] & 0x80) >> 7 ;
                    chip8->V[chip8->inst.X]<<=1;
                    break;
                
                default: break;
            }
            break;
        case 0x09:
            if(chip8->V[chip8->inst.X] != chip8->V[chip8->inst.Y])
                chip8->PC += 2;
            break;
        case 0x0A:
            //set index register i to NNN
            chip8->I = chip8->inst.NNN;
            break;
        
        case 0xB:
            chip8->PC = chip8->V[0] + chip8->inst.NNN;
            break;

        case 0xC:
            chip8->V[chip8->inst.X] = (rand()%256) & chip8->inst.NN;
            break;
    
        case 0x0D:
            //draw sprite at coords X,Y read from mem location I
            uint8_t X_coord = chip8->V[chip8->inst.X] % config.window_width;
            uint8_t Y_coord = chip8->V[chip8->inst.Y] % config.window_height;
            const uint8_t original_X = X_coord;
            chip8->V[0xF] = 0;

            for(uint8_t i = 0;i < chip8->inst.N;i++){
                const uint8_t sprite_data = chip8->ram[chip8->I + i];
                X_coord = original_X;
                for(int8_t j = 7;j >=0; j--){
                    bool *pixel = &chip8->display[Y_coord * config.window_width + X_coord];
                    const bool sprite_bit = (sprite_data & (1<<j));
                    if(sprite_bit && *pixel){
                        chip8->V[0xF]=1;
                    }

                    *pixel ^= sprite_bit;
                    if(++X_coord >= config.window_width) break;

                }
                if(++Y_coord >= config.window_height) break;
            }

            break;

            case 0x0E:
                if (chip8->inst.NN == 0x09E){
                    if(chip8->keypad[chip8->V[chip8->inst.X]]){
                        chip8->PC += 2; 
                    }
                } else if(chip8->inst.NN == 0x0A1){
                    if(!chip8->keypad[chip8->V[chip8->inst.X]]){
                        chip8->PC += 2; 
                    }
                }
                break;
            case 0x0F:
                switch(chip8->inst.NN){
                    case 0x0A:
                        bool any_key_pressed = false;
                        for(uint8_t i=0;i < sizeof chip8->keypad; i++){
                            if(chip8->keypad[i]){
                                chip8->V[chip8->inst.X] = i;
                                any_key_pressed = true;
                                break;
                            }   
                        }

                        if(!any_key_pressed){
                            chip8->PC -= 2;
                        }
                        break;

                    case 0x1E:
                        chip8->I = (chip8->I + chip8->V[chip8->inst.X]) & 0xFFF; 
                         break;

                    
                    case 0x07:
                        chip8->V[chip8->inst.X] = chip8->delay_timer;
                        break;
                    
                    case 0x15:
                        chip8->delay_timer = chip8->V[chip8->inst.X];
                        break;
                    
                    case 0x18:
                        chip8->sound_timer = chip8->V[chip8->inst.X];
                        break;
                    
                    case 0x29:
                        chip8->I = chip8->V[chip8->inst.X] * 5;
                        break;
                    
                    case 0x33:
                        uint8_t value = chip8->V[chip8->inst.X];
                        chip8->ram[chip8->I]     = value / 100;        // hundreds
                        chip8->ram[chip8->I + 1] = (value / 10) % 10;  // tens
                        chip8->ram[chip8->I + 2] = value % 10;         // ones
                        break;
                    
                    case 0x55:
                            for(uint8_t i = 0; i <= chip8->inst.X; i++) {
                                if (chip8->I + i < sizeof(chip8->ram)) {
                                    chip8->ram[chip8->I + i] = chip8->V[i];
                                }
                            }
                            break;

                    case 0x65:
                        for(uint8_t i = 0; i <= chip8->inst.X; i++) {
                            if (chip8->I + i < sizeof(chip8->ram)) {
                                chip8->V[i] = chip8->ram[chip8->I + i];
                            }
                        }
                        break;

                    default : break;
                }

        default: break;
    }
}

void clear_screen(sdl_t *sdl,const config_t config){
    const uint8_t r = (config.bgcolor >> 24) & 0xFF;
    const uint8_t g = (config.bgcolor >> 16) & 0xFF;
    const uint8_t b = (config.bgcolor >> 8) & 0xFF;
    const uint8_t a = config.bgcolor & 0xFF;
    SDL_SetRenderDrawColor(sdl->renderer,r,g,b,a);
    SDL_RenderClear(sdl->renderer);
}

void update_screen(const sdl_t sdl,const config_t config,const chip8_t chip8){
    SDL_Rect rect = {.x = 0, .y = 0, .w = config.scale_factor, .h = config.scale_factor};
    
    const uint8_t bg_r = (config.bgcolor >> 24) & 0xFF;
    const uint8_t bg_g = (config.bgcolor >> 16) & 0xFF;
    const uint8_t bg_b = (config.bgcolor >> 8) & 0xFF;
    const uint8_t bg_a = config.bgcolor & 0xFF;

    const uint8_t fg_r = (config.fgcolor >> 24) & 0xFF;
    const uint8_t fg_g = (config.fgcolor >> 16) & 0xFF;
    const uint8_t fg_b = (config.fgcolor >> 8) & 0xFF;
    const uint8_t fg_a = config.fgcolor & 0xFF;

    for(uint32_t i = 0; i < sizeof chip8.display; i++){
        rect.x = (i%config.window_width) * config.scale_factor;
        rect.y= (i/config .window_width) * config.scale_factor;

        if(chip8.display[i]){
            SDL_SetRenderDrawColor(sdl.renderer,fg_r,fg_g,fg_b,fg_a);
            SDL_RenderFillRect(sdl.renderer,&rect);
        } else {
            
            SDL_SetRenderDrawColor(sdl.renderer,bg_r,bg_g,bg_b,bg_a);
            SDL_RenderFillRect(sdl.renderer,&rect);
        }
    }
    SDL_RenderPresent(sdl.renderer);
}

//keypad
//123C --> 1234
//456D --> QWER
//789E --> ASDF
//A0BF --> ZXCV

void handle_input(chip8_t *chip8){
    SDL_Event event;
     while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                chip8->state = QUIT;
                return;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        chip8->state = QUIT;
                        return;

                    case SDLK_SPACE:   // pause toggle
                        if (chip8->state == RUNNING) {
                            chip8->state = PAUSED;
                        } else {
                            chip8->state = RUNNING;
                        }
                        puts("<=============PAUSED===============>");
                        break;

        
                    case SDLK_1: chip8->keypad[0x1] = true; break;
                    case SDLK_2: chip8->keypad[0x2] = true; break;
                    case SDLK_3: chip8->keypad[0x3] = true; break;
                    case SDLK_4: chip8->keypad[0xC] = true; break;

                    case SDLK_q: chip8->keypad[0x4] = true; break;
                    case SDLK_w: chip8->keypad[0x5] = true; break;
                    case SDLK_e: chip8->keypad[0x6] = true; break;
                    case SDLK_r: chip8->keypad[0xD] = true; break;

                    case SDLK_a: chip8->keypad[0x7] = true; break;
                    case SDLK_s: chip8->keypad[0x8] = true; break;
                    case SDLK_d: chip8->keypad[0x9] = true; break;
                    case SDLK_f: chip8->keypad[0xE] = true; break;

                    case SDLK_z: chip8->keypad[0xA] = true; break;
                    case SDLK_x: chip8->keypad[0x0] = true; break;
                    case SDLK_c: chip8->keypad[0xB] = true; break;
                    case SDLK_v: chip8->keypad[0xF] = true; break;

                    default: break;
                }
                break;

            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_1: chip8->keypad[0x1] = false; break;
                    case SDLK_2: chip8->keypad[0x2] = false; break;
                    case SDLK_3: chip8->keypad[0x3] = false; break;
                    case SDLK_4: chip8->keypad[0xC] = false; break;

                    case SDLK_q: chip8->keypad[0x4] = false; break;
                    case SDLK_w: chip8->keypad[0x5] = false; break;
                    case SDLK_e: chip8->keypad[0x6] = false; break;
                    case SDLK_r: chip8->keypad[0xD] = false; break;

                    case SDLK_a: chip8->keypad[0x7] = false; break;
                    case SDLK_s: chip8->keypad[0x8] = false; break;
                    case SDLK_d: chip8->keypad[0x9] = false; break;
                    case SDLK_f: chip8->keypad[0xE] = false; break;

                    case SDLK_z: chip8->keypad[0xA] = false; break;
                    case SDLK_x: chip8->keypad[0x0] = false; break;
                    case SDLK_c: chip8->keypad[0xB] = false; break;
                    case SDLK_v: chip8->keypad[0xF] = false; break;

                    default: break;
                }
                break;
        }
    }
}

void cleanup(sdl_t *sdl){

    SDL_DestroyRenderer(sdl->renderer);
    SDL_DestroyWindow(sdl->window);
    SDL_Quit();
}

void update_timers(chip8_t *chip8){
    if(chip8->delay_timer > 0){
        chip8->delay_timer--;
    }

    if(chip8->sound_timer > 0){
        chip8->sound_timer--;
    } else {
        
    }
}

int main(int argc,char **argv){ 
    sdl_t sdl = {0};
    chip8_t chip8 = {0};
    config_t config = {0};

    if(!set_config_from_args(&config,argc,argv)) exit(EXIT_FAILURE);
    //init SDL
    if(!init_sdl(&sdl,config)) exit(EXIT_FAILURE);
    char *rom_name = argv[1];
    if(!init_chip8(&chip8,rom_name)) exit(EXIT_FAILURE);
    clear_screen(&sdl,config);
    srand(time(NULL));
    //emu loop
    while(chip8.state != QUIT){
        handle_input(&chip8);
        if(chip8.state == PAUSED){
            continue;
        }
        
        uint64_t before_frame = SDL_GetPerformanceCounter();    
        for(uint32_t i=0;i<config.insts_per_second/60;i++)
            emu_instruction(&chip8,config);
        uint64_t after_frame = SDL_GetPerformanceCounter();

        double time_elapsed = (double)((after_frame - before_frame)*1000) / SDL_GetPerformanceFrequency();
        SDL_Delay(16.67f > time_elapsed ? 16.67 - time_elapsed : 0); //60fps
        
        #ifdef DEBUG
            print_debug_info(&chip8);
        #endif
        
        update_screen(sdl,config,chip8);
        update_timers(&chip8);
    }
    cleanup(&sdl);
    return 0;
}

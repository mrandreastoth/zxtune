//
//  state.h
//  vio2sf
//
//  Created by Christopher Snowhill on 10/13/13.
//  Copyright (c) 2013 Christopher Snowhill. All rights reserved.
//

#ifndef vio2sf_state_h
#define vio2sf_state_h

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SoundInterface_struct SoundInterface_struct;
    
typedef struct NDS_state
{
    // configuration
    unsigned long dwInterpolation;
    unsigned long dwChannelMute;
    
    // state setup info, from tags
	int initial_frames;
	int sync_type;
    int arm9_clockdown_level;
	int arm7_clockdown_level;
    
    u32 cycles;
    
    struct NDSSystem * nds;
    
    struct armcpu_t * NDS_ARM7;
    struct armcpu_t * NDS_ARM9;

    struct MMU_struct * MMU;
    
    struct ARM9_struct * ARM9Mem;
    
    u8 * MMU_ARM9_MEM_MAP[256];
    u32  MMU_ARM9_MEM_MASK[256];
    u8 * MMU_ARM7_MEM_MAP[256];
    u32  MMU_ARM7_MEM_MASK[256];

    BOOL execute;

    u16 partie; /* = 1; */
    
    u16 SPI_CNT; /* = 0;*/
    u16 SPI_CMD; /* = 0;*/
    u16 AUX_SPI_CNT; /* = 0;*/
    u16 AUX_SPI_CMD; /* = 0;*/
    
    u32 rom_mask; /* = 0;*/
    
    struct SPU_struct *SPU_core;
    
    SoundInterface_struct *SNDCore;
    
    s16 *sample_buffer;
    unsigned long sample_pointer;
    unsigned long sample_size;
} NDS_state;

int state_init(NDS_state *state);

void state_deinit(NDS_state *state);

void state_setrom(NDS_state *state, u8 * rom, u32 rom_size);

void state_loadstate(NDS_state *state, const u8 * ss, u32 ss_size);
    
void state_render(NDS_state *state, s16 * buffer, unsigned int sample_count);

#ifdef __cplusplus
};
#endif

#endif

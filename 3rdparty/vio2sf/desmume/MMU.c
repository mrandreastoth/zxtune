/*  Copyright (C) 2006 yopyop
    yopyop156@ifrance.com
    yopyop156.ifrance.com

	Copyright (C) 2007 shash

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//#define RENDER3D

#include <stdlib.h>
#include <math.h>
#include <string.h>

//#include "gl_vertex.h"
#include "spu_exports.h"

#include "state.h"

#include "MMU.h"

#include "debug.h"
#include "NDSSystem.h"
//#include "cflash.h"
#define cflash_read(s,a) 0
#define cflash_write(s,a,d)
#include "cp15.h"
#include "registers.h"
#include "isqrt.h"

#define ROM_MASK 3

/*
 *
 */
#define EARLY_MEMORY_ACCESS 1

#define INTERNAL_DTCM_READ 1
#define INTERNAL_DTCM_WRITE 1

#define DUP2(x)  x, x
#define DUP4(x)  x, x, x, x
#define DUP8(x)  x, x, x, x,  x, x, x, x
#define DUP16(x) x, x, x, x,  x, x, x, x,  x, x, x, x,  x, x, x, x

const u32 MMU_ARM9_WAIT16[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 1, 1, 1, 1, 1,
};

const u32 MMU_ARM9_WAIT32[16]={
	1, 1, 1, 1, 1, 2, 2, 1, 8, 8, 5, 1, 1, 1, 1, 1,
};

const u32 MMU_ARM7_WAIT16[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 1, 1, 1, 1, 1,
};

const u32 MMU_ARM7_WAIT32[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 8, 8, 5, 1, 1, 1, 1, 1,
};

void MMU_Init(NDS_state *state) {
	int i;

	LOG("MMU init\n");

	memset(state->MMU, 0, sizeof(MMU_struct));
  
  state->MMU->ARM7_MEM = calloc(0x34000, 1);
  state->MMU->ARM7_BIOS = state->MMU->ARM7_MEM;
  state->MMU->ARM7_ERAM = state->MMU->ARM7_BIOS + 0x4000;
  state->MMU->ARM7_REG = state->MMU->ARM7_ERAM + 0x10000;
  state->MMU->ARM7_WIRAM = state->MMU->ARM7_REG + 0x10000;
  
  state->MMU->SWIRAM = calloc(0x8000, 1);

	state->MMU->CART_ROM = state->MMU->UNUSED_RAM;
  state->MMU->CART_RAM = calloc(0x10000, 1);

        for(i = 0x80; i<0xA0; ++i)
        {
           state->MMU_ARM9_MEM_MAP[i] = state->MMU->CART_ROM;
           state->MMU_ARM7_MEM_MAP[i] = state->MMU->CART_ROM;
        }

	state->MMU->MMU_MEM[0] = state->MMU_ARM9_MEM_MAP;
	state->MMU->MMU_MEM[1] = state->MMU_ARM7_MEM_MAP;
	state->MMU->MMU_MASK[0]= state->MMU_ARM9_MEM_MASK;
	state->MMU->MMU_MASK[1] = state->MMU_ARM7_MEM_MASK;

	state->MMU->ITCMRegion = 0x00800000;

  state->MMU->fifos = calloc(16, sizeof(FIFO));
	for(i = 0;i < 16;i++)
		FIFOInit(state->MMU->fifos + i);
	
        mc_init(&state->MMU->fw, MC_TYPE_FLASH);  /* init fw device */
        mc_alloc(&state->MMU->fw, NDS_FW_SIZE_V1);

        // Init Backup Memory device, this should really be done when the rom is loaded
        mc_init(&state->MMU->bupmem, MC_TYPE_AUTODETECT);
        mc_alloc(&state->MMU->bupmem, 1);
}

void MMU_DeInit(NDS_state *state) {
	LOG("MMU deinit\n");
    mc_free(&state->MMU->fw);
    mc_free(&state->MMU->bupmem);
    free(state->MMU->fifos);
    state->MMU->fifos = 0;
    free(state->MMU->CART_RAM);
    state->MMU->CART_RAM = 0;
    free(state->MMU->SWIRAM);
    state->MMU->SWIRAM = 0;
    free(state->MMU->ARM7_MEM);
    state->MMU->ARM7_MEM = state->MMU->ARM7_BIOS = state->MMU->ARM7_ERAM = state->MMU->ARM7_REG = state->MMU->ARM7_WIRAM = 0;
}

//Card rom & ram


void MMU_clearMem(NDS_state *state)
{
	int i;

	memset(state->ARM9Mem->ARM9_ABG,  0, 0x080000);
	memset(state->ARM9Mem->ARM9_AOBJ, 0, 0x040000);
	memset(state->ARM9Mem->ARM9_BBG,  0, 0x020000);
	memset(state->ARM9Mem->ARM9_BOBJ, 0, 0x020000);
	memset(state->ARM9Mem->ARM9_DTCM, 0, 0x4000);
	memset(state->ARM9Mem->ARM9_ITCM, 0, 0x8000);
	memset(state->ARM9Mem->ARM9_LCD,  0, 0x0A4000);
	memset(state->ARM9Mem->ARM9_OAM,  0, 0x0800);
	memset(state->ARM9Mem->ARM9_REG,  0, 0x01000000);
	memset(state->ARM9Mem->ARM9_VMEM, 0, 0x0800);
	memset(state->ARM9Mem->ARM9_WRAM, 0, 0x01000000);
	memset(state->ARM9Mem->MAIN_MEM,  0, 0x400000);

	memset(state->ARM9Mem->blank_memory,  0, 0x020000);
	
	memset(state->MMU->ARM7_ERAM,     0, 0x010000);
	memset(state->MMU->ARM7_REG,      0, 0x010000);
	
	for(i = 0;i < 16;i++)
	FIFOInit(state->MMU->fifos + i);
	
	state->MMU->DTCMRegion = 0;
	state->MMU->ITCMRegion = 0x00800000;
	
	memset(state->MMU->timer,         0, sizeof(u16) * 2 * 4);
	memset(state->MMU->timerMODE,     0, sizeof(s32) * 2 * 4);
	memset(state->MMU->timerON,       0, sizeof(u32) * 2 * 4);
	memset(state->MMU->timerRUN,      0, sizeof(u32) * 2 * 4);
	memset(state->MMU->timerReload,   0, sizeof(u16) * 2 * 4);
	
	memset(state->MMU->reg_IME,       0, sizeof(u32) * 2);
	memset(state->MMU->reg_IE,        0, sizeof(u32) * 2);
	memset(state->MMU->reg_IF,        0, sizeof(u32) * 2);
	
	memset(state->MMU->DMAStartTime,  0, sizeof(u32) * 2 * 4);
	memset(state->MMU->DMACycle,      0, sizeof(s32) * 2 * 4);
	memset(state->MMU->DMACrt,        0, sizeof(u32) * 2 * 4);
	memset(state->MMU->DMAing,        0, sizeof(BOOL) * 2 * 4);
	
	memset(state->MMU->dscard,        0, sizeof(nds_dscard) * 2);
}

void MMU_setRom(NDS_state *state, u8 * rom, u32 mask)
{
	unsigned int i;
	state->MMU->CART_ROM = rom;
	
	for(i = 0x80; i<0xA0; ++i)
	{
		state->MMU_ARM9_MEM_MAP[i] = rom;
		state->MMU_ARM7_MEM_MAP[i] = rom;
		state->MMU_ARM9_MEM_MASK[i] = mask;
		state->MMU_ARM7_MEM_MASK[i] = mask;
	}
	state->rom_mask = mask;
}

void MMU_unsetRom(NDS_state *state)
{
	unsigned int i;
	state->MMU->CART_ROM=state->MMU->UNUSED_RAM;
	
	for(i = 0x80; i<0xA0; ++i)
	{
		state->MMU_ARM9_MEM_MAP[i] = state->MMU->UNUSED_RAM;
		state->MMU_ARM7_MEM_MAP[i] = state->MMU->UNUSED_RAM;
		state->MMU_ARM9_MEM_MASK[i] = ROM_MASK;
		state->MMU_ARM7_MEM_MASK[i] = ROM_MASK;
	}
	state->rom_mask = ROM_MASK;
}

u8 FASTCALL MMU_read8(NDS_state *state, u32 proc, u32 adr)
{
#ifdef INTERNAL_DTCM_READ
	if((proc==ARMCPU_ARM9)&((adr&(~0x3FFF))==state->MMU->DTCMRegion))
	{
		return state->ARM9Mem->ARM9_DTCM[adr&0x3FFF];
	}
#endif
	
	// CFlash reading, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000))
		return (unsigned char)cflash_read(state, adr);

  return state->MMU->MMU_MEM[proc][(adr>>20)&0xFF][adr&state->MMU->MMU_MASK[proc][(adr>>20)&0xFF]];
}



u16 FASTCALL MMU_read16(NDS_state *state, u32 proc, u32 adr)
{    
#ifdef INTERNAL_DTCM_READ
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Returns data from DTCM (ARM9 only) */
		return T1ReadWord(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF);
	}
#endif
	
	// CFlash reading, Mic
	if ((adr>=0x08800000)&&(adr<0x09900000))
	   return (unsigned short)cflash_read(state, adr);

	adr &= 0x0FFFFFFF;

	if(adr&0x04000000)
	{
		/* Adress is an IO register */
		switch(adr)
		{
			case REG_IPCFIFORECV :               /* TODO (clear): ??? */
				state->execute = FALSE;
				return 1;
				
			case REG_IME :
				return (u16)state->MMU->reg_IME[proc];
				
			case REG_IE :
				return (u16)state->MMU->reg_IE[proc];
			case REG_IE + 2 :
				return (u16)(state->MMU->reg_IE[proc]>>16);
				
			case REG_IF :
				return (u16)state->MMU->reg_IF[proc];
			case REG_IF + 2 :
				return (u16)(state->MMU->reg_IF[proc]>>16);
				
			case REG_TM0CNTL :
			case REG_TM1CNTL :
			case REG_TM2CNTL :
			case REG_TM3CNTL :
				return state->MMU->timer[proc][(adr&0xF)>>2];
			
			case 0x04000630 :
				LOG("vect res\r\n");	/* TODO (clear): ??? */
				//execute = FALSE;
				return 0;
                        case REG_POSTFLG :
				return 1;
			default :
				break;
		}
	}
	
    /* Returns data from memory */
	return T1ReadWord(state->MMU->MMU_MEM[proc][(adr >> 20) & 0xFF], adr & state->MMU->MMU_MASK[proc][(adr >> 20) & 0xFF]);
}

static u32 FASTCALL MMU_read32_io(NDS_state *state, u32 proc, u32 adr)
{
	switch(adr & 0xffffff)
	{
		// This is hacked due to the only current 3D core
		case 0x04000600 & 0xffffff:
          {
              u32 fifonum = IPCFIFO+proc;

			u32 gxstat =	(state->MMU->fifos[fifonum].empty<<26) |
							(1<<25) | 
							(state->MMU->fifos[fifonum].full<<24) |
							/*((NDS_nbpush[0]&1)<<13) | ((NDS_nbpush[2]&0x1F)<<8) |*/ 
							2;

			LOG ("GXSTAT: 0x%X", gxstat);

			return	gxstat;
          }

		case 0x04000640 & 0xffffff:
		case 0x04000644 & 0xffffff:
		case 0x04000648 & 0xffffff:
		case 0x0400064C & 0xffffff:
		case 0x04000650 & 0xffffff:
		case 0x04000654 & 0xffffff:
		case 0x04000658 & 0xffffff:
		case 0x0400065C & 0xffffff:
		case 0x04000660 & 0xffffff:
		case 0x04000664 & 0xffffff:
		case 0x04000668 & 0xffffff:
		case 0x0400066C & 0xffffff:
		case 0x04000670 & 0xffffff:
		case 0x04000674 & 0xffffff:
		case 0x04000678 & 0xffffff:
		case 0x0400067C & 0xffffff:
		case 0x04000680 & 0xffffff:
		case 0x04000684 & 0xffffff:
		case 0x04000688 & 0xffffff:
		case 0x0400068C & 0xffffff:
		case 0x04000690 & 0xffffff:
		case 0x04000694 & 0xffffff:
		case 0x04000698 & 0xffffff:
		case 0x0400069C & 0xffffff:
		case 0x040006A0 & 0xffffff:
		case 0x04000604 & 0xffffff:
		{
			return 0;
		}
		
		case REG_IME & 0xffffff :
			return state->MMU->reg_IME[proc];
		case REG_IE & 0xffffff :
			return state->MMU->reg_IE[proc];
		case REG_IF & 0xffffff :
			return state->MMU->reg_IF[proc];
		case REG_IPCFIFORECV & 0xffffff :
		{
			u16 IPCFIFO_CNT = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x184);
			if(IPCFIFO_CNT&0x8000)
			{
			//execute = FALSE;
			u32 fifonum = IPCFIFO+proc;
			u32 val = FIFOValue(state->MMU->fifos + fifonum);
			u32 remote = (proc+1) & 1;
			u16 IPCFIFO_CNT_remote = T1ReadWord(state->MMU->MMU_MEM[remote][0x40], 0x184);
			IPCFIFO_CNT |= (state->MMU->fifos[fifonum].empty<<8) | (state->MMU->fifos[fifonum].full<<9) | (state->MMU->fifos[fifonum].error<<14);
			IPCFIFO_CNT_remote |= (state->MMU->fifos[fifonum].empty) | (state->MMU->fifos[fifonum].full<<1);
			T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, IPCFIFO_CNT);
			T1WriteWord(state->MMU->MMU_MEM[remote][0x40], 0x184, IPCFIFO_CNT_remote);
			if ((state->MMU->fifos[fifonum].empty) && (IPCFIFO_CNT & BIT(2)))
				NDS_makeInt(state, remote,17) ; /* remote: SEND FIFO EMPTY */
			return val;
			}
		}
		return 0;
                      case REG_TM0CNTL & 0xffffff :
                      case REG_TM1CNTL & 0xffffff :
                      case REG_TM2CNTL & 0xffffff :
                      case REG_TM3CNTL & 0xffffff :
		{
			u32 val = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], (adr + 2) & 0xFFF);
			return state->MMU->timer[proc][(adr&0xF)>>2] | (val<<16);
		}	
                      case REG_GCDATAIN & 0xffffff:
		{
                              u32 val;

                              if(!state->MMU->dscard[proc].adress) return 0;

                              val = T1ReadLong(state->MMU->CART_ROM, state->MMU->dscard[proc].adress);

			state->MMU->dscard[proc].adress += 4;	/* increment adress */
			
			state->MMU->dscard[proc].transfer_count--;	/* update transfer counter */
			if(state->MMU->dscard[proc].transfer_count) /* if transfer is not ended */
			{
				return val;	/* return data */
			}
			else	/* transfer is done */
                              {                                                       
                                      T1WriteLong(state->MMU->MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, T1ReadLong(state->MMU->MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff) & ~(0x00800000 | 0x80000000));
				/* = 0x7f7fffff */
				
				/* if needed, throw irq for the end of transfer */
                                      if(T1ReadWord(state->MMU->MMU_MEM[proc][(REG_AUXSPICNT >> 20) & 0xff], REG_AUXSPICNT & 0xfff) & 0x4000)
				{
                                              if(proc == ARMCPU_ARM7) NDS_makeARM7Int(state, 19);
                                              else NDS_makeARM9Int(state, 19);
				}
				
				return val;
			}
		}

		default :
			break;
	}
	
	/* Returns data from memory */
	return T1ReadLong(state->MMU->MMU_MEM[proc][(adr >> 20) & 0xFF], adr & state->MMU->MMU_MASK[proc][(adr >> 20) & 0xFF]);
}
	 
u32 FASTCALL MMU_read32(NDS_state *state, u32 proc, u32 adr)
{
#ifdef INTERNAL_DTCM_READ
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Returns data from DTCM (ARM9 only) */
		return T1ReadLong(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF);
	}
#endif
	
	// CFlash reading, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000))
	   return (unsigned long)cflash_read(state, adr);
		
	adr &= 0x0FFFFFFF;

	if((adr >> 24) == 4)
	{
		/* Adress is an IO register */
    return MMU_read32_io(state, proc, adr);
  }
  else
  {
    return T1ReadLong(state->MMU->MMU_MEM[proc][(adr >> 20) & 0xFF], adr & state->MMU->MMU_MASK[proc][(adr >> 20) & 0xFF]);
  }
}
	
void FASTCALL MMU_write8(NDS_state *state, u32 proc, u32 adr, u8 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Writes data in DTCM (ARM9 only) */
		state->ARM9Mem->ARM9_DTCM[adr&0x3FFF] = val;
		return ;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000)) {
		cflash_write(state,adr,val);
		return;
	}

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteByte(state, adr, val);
              return;
           }
        }

	if ((adr & 0xFF800000) == 0x04800000)
	{
		/* is wifi hardware, dont intermix with regular hardware registers */
		/* FIXME handle 8 bit writes */
		return ;
	}
	state->MMU->MMU_MEM[proc][(adr>>20)&0xFF][adr&state->MMU->MMU_MASK[proc][(adr>>20)&0xFF]]=val;
}

void FASTCALL MMU_write16(NDS_state *state, u32 proc, u32 adr, u16 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Writes in DTCM (ARM9 only) */
		T1WriteWord(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF, val);
		return;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x08800000)&&(adr<0x09900000))
	{
		cflash_write(state,adr,val);
		return;
	}

	if ((proc==ARMCPU_ARM7) && (adr>=0x04800000)&&(adr<0x05000000))
		return ;

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteWord(state, adr, val);
              return;
           }
        }

	if((adr >> 24) == 4)
	{
		/* Adress is an IO register */
		switch(adr)
		{
      case REG_POWCNT1 :
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x304, val);
				return;

                        case REG_AUXSPICNT:
                                T1WriteWord(state->MMU->MMU_MEM[proc][(REG_AUXSPICNT >> 20) & 0xff], REG_AUXSPICNT & 0xfff, val);
                                state->AUX_SPI_CNT = val;

                                if (val == 0)
                                   mc_reset_com(&state->MMU->bupmem);     /* reset backup memory device communication */
				return;
				
                        case REG_AUXSPIDATA:
                                if(val!=0)
                                {
                                   state->AUX_SPI_CMD = val & 0xFF;
                                }

                                T1WriteWord(state->MMU->MMU_MEM[proc][(REG_AUXSPIDATA >> 20) & 0xff], REG_AUXSPIDATA & 0xfff, bm_transfer(&state->MMU->bupmem, (u8)val));
				return;

			case REG_SPICNT :
				if(proc == ARMCPU_ARM7)
				{
                                  int reset_firmware = 1;

                                  if ( ((state->SPI_CNT >> 8) & 0x3) == 1) {
                                    if ( ((val >> 8) & 0x3) == 1) {
                                      if ( BIT11(state->SPI_CNT)) {
                                        /* select held */
                                        reset_firmware = 0;
                                      }
                                    }
                                  }
					
                                        //MMU->fw.com == 0; /* reset fw device communication */
                                    if ( reset_firmware) {
                                      /* reset fw device communication */
                                      mc_reset_com(&state->MMU->fw);
                                    }
                                    state->SPI_CNT = val;
                                }
				
				T1WriteWord(state->MMU->MMU_MEM[proc][(REG_SPICNT >> 20) & 0xff], REG_SPICNT & 0xfff, val);
				return;
				
			case REG_SPIDATA :
				if(proc==ARMCPU_ARM7)
				{
                                        u16 spicnt;

					if(val!=0)
					{
						state->SPI_CMD = val;
					}
			
                                        spicnt = T1ReadWord(state->MMU->MMU_MEM[proc][(REG_SPICNT >> 20) & 0xff], REG_SPICNT & 0xfff);
					
                                        switch((spicnt >> 8) & 0x3)
					{
                                                case 0 :
							break;
							
                                                case 1 : /* firmware memory device */
                                                        if((spicnt & 0x3) != 0)      /* check SPI baudrate (must be 4mhz) */
							{
								T1WriteWord(state->MMU->MMU_MEM[proc][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, 0);
								break;
							}
							T1WriteWord(state->MMU->MMU_MEM[proc][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, fw_transfer(&state->MMU->fw, (u8)val));

							return;
							
                                                case 2 :
							switch(state->SPI_CMD & 0x70)
							{
								case 0x00 :
									val = 0;
									break;
								case 0x10 :
									//execute = FALSE;
									if(state->SPI_CNT&(1<<11))
									{
										if(state->partie)
										{
											val = ((state->nds->touchY<<3)&0x7FF);
											state->partie = 0;
											//execute = FALSE;
											break;
										}
										val = (state->nds->touchY>>5);
                                                                                state->partie = 1;
										break;
									}
									val = ((state->nds->touchY<<3)&0x7FF);
									state->partie = 1;
									break;
								case 0x20 :
									val = 0;
									break;
								case 0x30 :
									val = 0;
									break;
								case 0x40 :
									val = 0;
									break;
								case 0x50 :
                                                                        if(spicnt & 0x800)
									{
										if(state->partie)
										{
											val = ((state->nds->touchX<<3)&0x7FF);
											state->partie = 0;
											break;
										}
										val = (state->nds->touchX>>5);
										state->partie = 1;
										break;
									}
									val = ((state->nds->touchX<<3)&0x7FF);
									state->partie = 1;
									break;
								case 0x60 :
									val = 0;
									break;
								case 0x70 :
									val = 0;
									break;
							}
							break;
							
                                                case 3 :
							/* NOTICE: Device 3 of SPI is reserved (unused and unusable) */
							break;
					}
				}
				
				T1WriteWord(state->MMU->MMU_MEM[proc][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, val);
				return;
				
				/* NOTICE: Perhaps we have to use gbatek-like reg names instead of libnds-like ones ...*/
				
                        case REG_DISPA_BG0CNT :
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x8, val);
				return;
                        case REG_DISPA_BG1CNT :
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xA, val);
				return;
                        case REG_DISPA_BG2CNT :
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xC, val);
				return;
                        case REG_DISPA_BG3CNT :
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xE, val);
				return;
                        case REG_DISPB_BG0CNT :
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x1008, val);
				return;
                        case REG_DISPB_BG1CNT :
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x100A, val);
				return;
                        case REG_DISPB_BG2CNT :
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x100C, val);
				return;
                        case REG_DISPB_BG3CNT :
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x100E, val);
				return;
                        case REG_IME : {
			        u32 old_val = state->MMU->reg_IME[proc];
				u32 new_val = val & 1;
				state->MMU->reg_IME[proc] = new_val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x208, val);
				if ( new_val && old_val != new_val) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( state->MMU->reg_IE[proc] & state->MMU->reg_IF[proc]) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			}
			case REG_VRAMCNTA:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTC:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTE:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTG:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTI:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				return ;

			case REG_IE :
				state->MMU->reg_IE[proc] = (state->MMU->reg_IE[proc]&0xFFFF0000) | val;
				if ( state->MMU->reg_IME[proc]) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( state->MMU->reg_IE[proc] & state->MMU->reg_IF[proc]) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			case REG_IE + 2 :
				state->execute = FALSE;
				state->MMU->reg_IE[proc] = (state->MMU->reg_IE[proc]&0xFFFF) | (((u32)val)<<16);
				return;
				
			case REG_IF :
				state->execute = FALSE;
				state->MMU->reg_IF[proc] &= (~((u32)val));
				return;
			case REG_IF + 2 :
				state->execute = FALSE;
				state->MMU->reg_IF[proc] &= (~(((u32)val)<<16));
				return;
				
                        case REG_IPCSYNC :
				{
				u32 remote = (proc+1)&1;
				u16 IPCSYNC_remote = T1ReadWord(state->MMU->MMU_MEM[remote][0x40], 0x180);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x180, (val&0xFFF0)|((IPCSYNC_remote>>8)&0xF));
				T1WriteWord(state->MMU->MMU_MEM[remote][0x40], 0x180, (IPCSYNC_remote&0xFFF0)|((val>>8)&0xF));
				state->MMU->reg_IF[remote] |= ((IPCSYNC_remote & (1<<14))<<2) & ((val & (1<<13))<<3);// & (MMU->reg_IME[remote] << 16);// & (MMU->reg_IE[remote] & (1<<16));//
				//execute = FALSE;
				}
				return;
                        case REG_IPCFIFOCNT :
				{
					u32 cnt_l = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x184) ;
					u32 cnt_r = T1ReadWord(state->MMU->MMU_MEM[(proc+1) & 1][0x40], 0x184) ;
					if ((val & 0x8000) && !(cnt_l & 0x8000))
					{
						/* this is the first init, the other side didnt init yet */
						/* so do a complete init */
						FIFOInit(state->MMU->fifos + (IPCFIFO+proc));
						T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184,0x8101) ;
						/* and then handle it as usual */
					}

				if(val & 0x4008)
				{
					FIFOInit(state->MMU->fifos + (IPCFIFO+((proc+1)&1)));
					T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, (cnt_l & 0x0301) | (val & 0x8404) | 1);
					T1WriteWord(state->MMU->MMU_MEM[proc^1][0x40], 0x184, (cnt_r & 0xC507) | 0x100);
					state->MMU->reg_IF[proc] |= ((val & 4)<<15);// & (MMU->reg_IME[proc]<<17);// & (MMU->reg_IE[proc]&0x20000);//
					return;
				}
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x184) | (val & 0xBFF4));
				}
				return;
                        case REG_TM0CNTL :
                        case REG_TM1CNTL :
                        case REG_TM2CNTL :
                        case REG_TM3CNTL :
				state->MMU->timerReload[proc][(adr>>2)&3] = val;
				return;
                        case REG_TM0CNTH :
                        case REG_TM1CNTH :
                        case REG_TM2CNTH :
                        case REG_TM3CNTH :
				if(val&0x80)
				{
				  state->MMU->timer[proc][((adr-2)>>2)&0x3] = state->MMU->timerReload[proc][((adr-2)>>2)&0x3];
				}
				state->MMU->timerON[proc][((adr-2)>>2)&0x3] = val & 0x80;
				switch(val&7)
				{
				case 0 :
					state->MMU->timerMODE[proc][((adr-2)>>2)&0x3] = 0+1;//proc;
					break;
				case 1 :
					state->MMU->timerMODE[proc][((adr-2)>>2)&0x3] = 6+1;//proc;
					break;
				case 2 :
					state->MMU->timerMODE[proc][((adr-2)>>2)&0x3] = 8+1;//proc;
					break;
				case 3 :
					state->MMU->timerMODE[proc][((adr-2)>>2)&0x3] = 10+1;//proc;
					break;
				default :
					state->MMU->timerMODE[proc][((adr-2)>>2)&0x3] = 0xFFFF;
					break;
				}
				if(!(val & 0x80))
				state->MMU->timerRUN[proc][((adr-2)>>2)&0x3] = FALSE;
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], adr & 0xFFF, val);
				return;
                        case REG_DISPA_DISPCNT+2 : 
				{
				//execute = FALSE;
				u32 v = (T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0) & 0xFFFF) | ((u32) val << 16);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0, v);
				}
				return;
                        case REG_DISPA_DISPCNT :
				if(proc == ARMCPU_ARM9)
				{
				u32 v = (T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0) & 0xFFFF0000) | val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0, v);
				}
				return;
                        case REG_DISPA_DISPCAPCNT :
				return;
                        case REG_DISPB_DISPCNT+2 : 
				if(proc == ARMCPU_ARM9)
				{
				//execute = FALSE;
				u32 v = (T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x1000) & 0xFFFF) | ((u32) val << 16);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x1000, v);
				}
				return;
                        case REG_DISPB_DISPCNT :
				{
				u32 v = (T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x1000) & 0xFFFF0000) | val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x1000, v);
				}
				return;
			//case 0x020D8460 :
			/*case 0x0235A904 :
				LOG("ECRIRE %d %04X\r\n", proc, val);
				execute = FALSE;*/
                                case REG_DMA0CNTH :
				{
                                u32 v;

				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma0 %04X\r\n", val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xBA, val);
				state->MMU->DMASrc[proc][0] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB0);
				state->MMU->DMADst[proc][0] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB4);
                                v = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB8);
				state->MMU->DMAStartTime[proc][0] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				state->MMU->DMACrt[proc][0] = v;
				if(state->MMU->DMAStartTime[proc][0] == 0)
					MMU_doDMA(state, proc, 0);
				}
				return;
                                case REG_DMA1CNTH :
				{
                                u32 v;
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma1 %04X\r\n", val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xC6, val);
				state->MMU->DMASrc[proc][1] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xBC);
				state->MMU->DMADst[proc][1] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xC0);
                                v = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xC4);
				state->MMU->DMAStartTime[proc][1] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				state->MMU->DMACrt[proc][1] = v;
				if(state->MMU->DMAStartTime[proc][1] == 0)
					MMU_doDMA(state, proc, 1);
				}
				return;
                                case REG_DMA2CNTH :
				{
                                u32 v;
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma2 %04X\r\n", val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xD2, val);
				state->MMU->DMASrc[proc][2] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xC8);
				state->MMU->DMADst[proc][2] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xCC);
                                v = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xD0);
				state->MMU->DMAStartTime[proc][2] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				state->MMU->DMACrt[proc][2] = v;
				if(state->MMU->DMAStartTime[proc][2] == 0)
					MMU_doDMA(state, proc, 2);
				}
				return;
                                case REG_DMA3CNTH :
				{
                                u32 v;
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma3 %04X\r\n", val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xDE, val);
				state->MMU->DMASrc[proc][3] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xD4);
				state->MMU->DMADst[proc][3] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xD8);
                                v = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xDC);
				state->MMU->DMAStartTime[proc][3] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				state->MMU->DMACrt[proc][3] = v;
		
				if(state->MMU->DMAStartTime[proc][3] == 0)
					MMU_doDMA(state, proc, 3);
				}
				return;
                        //case REG_AUXSPICNT : execute = FALSE;
			default :
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], adr&state->MMU->MMU_MASK[proc][(adr>>20)&0xFF], val);
				return;
		}
	}
	T1WriteWord(state->MMU->MMU_MEM[proc][(adr>>20)&0xFF], adr&state->MMU->MMU_MASK[proc][(adr>>20)&0xFF], val);
} 


void FASTCALL MMU_write32(NDS_state *state, u32 proc, u32 adr, u32 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc==ARMCPU_ARM9)&((adr&(~0x3FFF))==state->MMU->DTCMRegion))
	{
		T1WriteLong(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF, val);
		return ;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000)) {
	   cflash_write(state,adr,val);
	   return;
	}

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteLong(state, adr, val);
              return;
           }
        }

		if ((adr & 0xFF800000) == 0x04800000) {
		/* access to non regular hw registers */
		/* return to not overwrite valid data */
			return ;
		} ;


	if((adr>>24)==4)
	{
		if (adr >= 0x04000400 && adr < 0x04000440)
		{
			// Geometry commands (aka Dislay Lists) - Parameters:X
			((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x400>>2] = val;
		}
		else
		switch(adr)
		{
			case REG_DISPA_WININ: 	 
			case REG_DISPB_WININ:
			case REG_DISPA_BLDCNT:
			case REG_DISPB_BLDCNT:
				break;
      case REG_DISPA_DISPCNT :
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0, val);
				return;
				
      case REG_DISPB_DISPCNT : 
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x1000, val);
				return;
			case REG_VRAMCNTA:
			case REG_VRAMCNTE:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				MMU_write8(state,proc,adr+2,val >> 16) ;
				MMU_write8(state,proc,adr+3,val >> 24) ;
				return ;
			case REG_VRAMCNTI:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				return ;

                        case REG_IME : {
			        u32 old_val = state->MMU->reg_IME[proc];
				u32 new_val = val & 1;
				state->MMU->reg_IME[proc] = new_val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x208, val);
				if ( new_val && old_val != new_val) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( state->MMU->reg_IE[proc] & state->MMU->reg_IF[proc]) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			}
				
			case REG_IE :
				state->MMU->reg_IE[proc] = val;
				if ( state->MMU->reg_IME[proc]) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( state->MMU->reg_IE[proc] & state->MMU->reg_IF[proc]) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			
			case REG_IF :
				state->MMU->reg_IF[proc] &= (~val);
				return;
                        case REG_TM0CNTL :
                        case REG_TM1CNTL :
                        case REG_TM2CNTL :
                        case REG_TM3CNTL :
				state->MMU->timerReload[proc][(adr>>2)&0x3] = (u16)val;
				if(val&0x800000)
				{
					state->MMU->timer[proc][(adr>>2)&0x3] = state->MMU->timerReload[proc][(adr>>2)&0x3];
				}
				state->MMU->timerON[proc][(adr>>2)&0x3] = val & 0x800000;
				switch((val>>16)&7)
				{
					case 0 :
					state->MMU->timerMODE[proc][(adr>>2)&0x3] = 0+1;//proc;
					break;
					case 1 :
					state->MMU->timerMODE[proc][(adr>>2)&0x3] = 6+1;//proc;
					break;
					case 2 :
					state->MMU->timerMODE[proc][(adr>>2)&0x3] = 8+1;//proc;
					break;
					case 3 :
					state->MMU->timerMODE[proc][(adr>>2)&0x3] = 10+1;//proc;
					break;
					default :
					state->MMU->timerMODE[proc][(adr>>2)&0x3] = 0xFFFF;
					break;
				}
				if(!(val & 0x800000))
				{
					state->MMU->timerRUN[proc][(adr>>2)&0x3] = FALSE;
				}
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], adr & 0xFFF, val);
				return;
                        case REG_DIVDENOM :
				{
                                        u16 cnt;
					s64 num = 0;
					s64 den = 1;
					s64 res;
					s64 mod;
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x298, val);
                                        cnt = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x280);
					switch(cnt&3)
					{
					case 0:
					{
						num = (s64) (s32) T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x290);
						den = (s64) (s32) T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x298);
					}
					break;
					case 1:
					{
						num = (s64) T1ReadQuad(state->MMU->MMU_MEM[proc][0x40], 0x290);
						den = (s64) (s32) T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x298);
					}
					break;
					case 2:
					{
						return;
					}
					break;
					default: 
						break;
					}
					if(den==0)
					{
						res = 0;
						mod = 0;
						cnt |= 0x4000;
						cnt &= 0x7FFF;
					}
					else
					{
						res = num / den;
						mod = num % den;
						cnt &= 0x3FFF;
					}
					DIVLOG("BOUT1 %08X%08X / %08X%08X = %08X%08X\r\n", (u32)(num>>32), (u32)num, 
											(u32)(den>>32), (u32)den, 
											(u32)(res>>32), (u32)res);
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A0, (u32) res);
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A4, (u32) (res >> 32));
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A8, (u32) mod);
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2AC, (u32) (mod >> 32));
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x280, cnt);
				}
				return;
                        case REG_DIVDENOM+4 :
			{
                                u16 cnt;
				s64 num = 0;
				s64 den = 1;
				s64 res;
				s64 mod;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x29C, val);
                                cnt = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x280);
				switch(cnt&3)
				{
				case 0:
				{
					return;
				}
				break;
				case 1:
				{
					return;
				}
				break;
				case 2:
				{
					num = (s64) T1ReadQuad(state->MMU->MMU_MEM[proc][0x40], 0x290);
					den = (s64) T1ReadQuad(state->MMU->MMU_MEM[proc][0x40], 0x298);
				}
				break;
				default: 
					break;
				}
				if(den==0)
				{
					res = 0;
					mod = 0;
					cnt |= 0x4000;
					cnt &= 0x7FFF;
				}
				else
				{
					res = num / den;
					mod = num % den;
					cnt &= 0x3FFF;
				}
				DIVLOG("BOUT2 %08X%08X / %08X%08X = %08X%08X\r\n", (u32)(num>>32), (u32)num, 
										(u32)(den>>32), (u32)den, 
										(u32)(res>>32), (u32)res);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A0, (u32) res);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A4, (u32) (res >> 32));
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A8, (u32) mod);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2AC, (u32) (mod >> 32));
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x280, cnt);
			}
			return;
                        case REG_SQRTPARAM :
				{
                                        u16 cnt;
					u64 v = 1;
					//execute = FALSE;
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2B8, val);
                                        cnt = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x2B0);
					switch(cnt&1)
					{
					case 0:
						v = (u64) T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x2B8);
						break;
					case 1:
						return;
					}
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2B4, (u32) isqrt64(v));
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2B0, cnt & 0x7FFF);
					SQRTLOG("BOUT1 sqrt(%08X%08X) = %08X\r\n", (u32)(v>>32), (u32)v, 
										T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x2B4));
				}
				return;
                        case REG_SQRTPARAM+4 :
				{
                                        u16 cnt;
					u64 v = 1;
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2BC, val);
                                        cnt = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x2B0);
					switch(cnt&1)
					{
					case 0:
						return;
						//break;
					case 1:
						v = T1ReadQuad(state->MMU->MMU_MEM[proc][0x40], 0x2B8);
						break;
					}
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2B4, (u32) isqrt64(v));
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2B0, cnt & 0x7FFF);
					SQRTLOG("BOUT2 sqrt(%08X%08X) = %08X\r\n", (u32)(v>>32), (u32)v, 
										T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x2B4));
				}
				return;
                        case REG_IPCSYNC :
				{
					//execute=FALSE;
					u32 remote = (proc+1)&1;
					u32 IPCSYNC_remote = T1ReadLong(state->MMU->MMU_MEM[remote][0x40], 0x180);
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x180, (val&0xFFF0)|((IPCSYNC_remote>>8)&0xF));
					T1WriteLong(state->MMU->MMU_MEM[remote][0x40], 0x180, (IPCSYNC_remote&0xFFF0)|((val>>8)&0xF));
					state->MMU->reg_IF[remote] |= ((IPCSYNC_remote & (1<<14))<<2) & ((val & (1<<13))<<3);// & (MMU->reg_IME[remote] << 16);// & (MMU->reg_IE[remote] & (1<<16));//
				}
				return;
                        case REG_IPCFIFOCNT :
							{
					u32 cnt_l = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x184) ;
					u32 cnt_r = T1ReadWord(state->MMU->MMU_MEM[(proc+1) & 1][0x40], 0x184) ;
					if ((val & 0x8000) && !(cnt_l & 0x8000))
					{
						/* this is the first init, the other side didnt init yet */
						/* so do a complete init */
						FIFOInit(state->MMU->fifos + (IPCFIFO+proc));
						T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184,0x8101) ;
						/* and then handle it as usual */
					}
				if(val & 0x4008)
				{
					FIFOInit(state->MMU->fifos + (IPCFIFO+((proc+1)&1)));
					T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, (cnt_l & 0x0301) | (val & 0x8404) | 1);
					T1WriteWord(state->MMU->MMU_MEM[proc^1][0x40], 0x184, (cnt_r & 0xC507) | 0x100);
					state->MMU->reg_IF[proc] |= ((val & 4)<<15);// & (MMU->reg_IME[proc]<<17);// & (MMU->reg_IE[proc]&0x20000);//
					return;
				}
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, val & 0xBFF4);
				//execute = FALSE;
				return;
							}
                        case REG_IPCFIFOSEND :
				{
					u16 IPCFIFO_CNT = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x184);
					if(IPCFIFO_CNT&0x8000)
					{
					//if(val==43) execute = FALSE;
					u32 remote = (proc+1)&1;
					u32 fifonum = IPCFIFO+remote;
                                        u16 IPCFIFO_CNT_remote;
					FIFOAdd(state->MMU->fifos + fifonum, val);
					IPCFIFO_CNT = (IPCFIFO_CNT & 0xFFFC) | (state->MMU->fifos[fifonum].full<<1);
                                        IPCFIFO_CNT_remote = T1ReadWord(state->MMU->MMU_MEM[remote][0x40], 0x184);
					IPCFIFO_CNT_remote = (IPCFIFO_CNT_remote & 0xFCFF) | (state->MMU->fifos[fifonum].full<<10);
					T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, IPCFIFO_CNT);
					T1WriteWord(state->MMU->MMU_MEM[remote][0x40], 0x184, IPCFIFO_CNT_remote);
					state->MMU->reg_IF[remote] |= ((IPCFIFO_CNT_remote & (1<<10))<<8);// & (MMU->reg_IME[remote] << 18);// & (MMU->reg_IE[remote] & 0x40000);//
					//execute = FALSE;
					}
				}
				return;
			case REG_DMA0CNTL :
				//LOG("32 bit dma0 %04X\r\n", val);
				state->MMU->DMASrc[proc][0] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB0);
				state->MMU->DMADst[proc][0] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB4);
				state->MMU->DMAStartTime[proc][0] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				state->MMU->DMACrt[proc][0] = val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0xB8, val);
				if( state->MMU->DMAStartTime[proc][0] == 0 ||
					state->MMU->DMAStartTime[proc][0] == 7)		// Start Immediately
					MMU_doDMA(state, proc, 0);
				return;
			case REG_DMA1CNTL:
				//LOG("32 bit dma1 %04X\r\n", val);
				state->MMU->DMASrc[proc][1] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xBC);
				state->MMU->DMADst[proc][1] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xC0);
				state->MMU->DMAStartTime[proc][1] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				state->MMU->DMACrt[proc][1] = val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0xC4, val);
				if(state->MMU->DMAStartTime[proc][1] == 0 ||
					state->MMU->DMAStartTime[proc][1] == 7)		// Start Immediately
					MMU_doDMA(state, proc, 1);
				return;
			case REG_DMA2CNTL :
				//LOG("32 bit dma2 %04X\r\n", val);
				state->MMU->DMASrc[proc][2] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xC8);
				state->MMU->DMADst[proc][2] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xCC);
				state->MMU->DMAStartTime[proc][2] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				state->MMU->DMACrt[proc][2] = val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0xD0, val);
				if(state->MMU->DMAStartTime[proc][2] == 0 ||
					state->MMU->DMAStartTime[proc][2] == 7)		// Start Immediately
					MMU_doDMA(state, proc, 2);
				return;
			case 0x040000DC :
				//LOG("32 bit dma3 %04X\r\n", val);
				state->MMU->DMASrc[proc][3] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xD4);
				state->MMU->DMADst[proc][3] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xD8);
				state->MMU->DMAStartTime[proc][3] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				state->MMU->DMACrt[proc][3] = val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0xDC, val);
				if(	state->MMU->DMAStartTime[proc][3] == 0 ||
					state->MMU->DMAStartTime[proc][3] == 7)		// Start Immediately
					MMU_doDMA(state, proc, 3);
				return;
                        case REG_GCROMCTRL :
				{
					int i;

                                        if(MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT) == 0xB7)
					{
                                                state->MMU->dscard[proc].adress = (MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT+1) << 24) | (MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT+2) << 16) | (MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT+3) << 8) | (MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT+4));
						state->MMU->dscard[proc].transfer_count = 0x80;// * ((val>>24)&7));
					}
                                        else if (MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT) == 0xB8)
                                        {
                                                // Get ROM chip ID
                                                val |= 0x800000; // Data-Word Status
                                                T1WriteLong(state->MMU->MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
                                                state->MMU->dscard[proc].adress = 0;
                                        }
					else
					{
                                                LOG("CARD command: %02X\n", MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT));
					}
					
					//CARDLOG("%08X : %08X %08X\r\n", adr, val, adresse[proc]);
                    val |= 0x00800000;
					
					if(state->MMU->dscard[proc].adress == 0)
					{
                                                val &= ~0x80000000; 
                                                T1WriteLong(state->MMU->MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
						return;
					}
                                        T1WriteLong(state->MMU->MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
										
					/* launch DMA if start flag was set to "DS Cart" */
					if(proc == ARMCPU_ARM7) i = 2;
					else i = 5;
					
					if(proc == ARMCPU_ARM9 && state->MMU->DMAStartTime[proc][0] == i)	/* dma0/1 on arm7 can't start on ds cart event */
					{
						MMU_doDMA(state, proc, 0);
						return;
					}
					else if(proc == ARMCPU_ARM9 && state->MMU->DMAStartTime[proc][1] == i)
					{
						MMU_doDMA(state, proc, 1);
						return;
					}
					else if(state->MMU->DMAStartTime[proc][2] == i)
					{
						MMU_doDMA(state, proc, 2);
						return;
					}
					else if(state->MMU->DMAStartTime[proc][3] == i)
					{
						MMU_doDMA(state, proc, 3);
						return;
					}
					return;

				}
				return;
								case REG_DISPA_DISPCAPCNT :
				if(proc == ARMCPU_ARM9)
				{
					T1WriteLong(state->ARM9Mem->ARM9_REG, 0x64, val);
				}
				return;
				
                        case REG_DISPA_BG0CNT :
				T1WriteLong(state->ARM9Mem->ARM9_REG, 8, val);
				return;
                        case REG_DISPA_BG2CNT :
				T1WriteLong(state->ARM9Mem->ARM9_REG, 0xC, val);
				return;
                        case REG_DISPB_BG0CNT :
				T1WriteLong(state->ARM9Mem->ARM9_REG, 0x1008, val);
				return;
                        case REG_DISPB_BG2CNT :
				T1WriteLong(state->ARM9Mem->ARM9_REG, 0x100C, val);
				return;
			case REG_DISPA_DISPMMEMFIFO:
			{
				// NOTE: right now, the capture unit is not taken into account,
				//       I don't know is it should be handled here or 
			
				FIFOAdd(state->MMU->fifos + MAIN_MEMORY_DISP_FIFO, val);
				break;
			}
			//case 0x21FDFF0 :  if(val==0) execute = FALSE;
			//case 0x21FDFB0 :  if(val==0) execute = FALSE;
			default :
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], adr & state->MMU->MMU_MASK[proc][(adr>>20)&0xFF], val);
				return;
		}
	}
	T1WriteLong(state->MMU->MMU_MEM[proc][(adr>>20)&0xFF], adr&state->MMU->MMU_MASK[proc][(adr>>20)&0xFF], val);
}


void FASTCALL MMU_doDMA(NDS_state *state, u32 proc, u32 num)
{
	u32 src = state->MMU->DMASrc[proc][num];
	u32 dst = state->MMU->DMADst[proc][num];
        u32 taille;

	if(src==dst)
	{
		T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0xB8 + (0xC*num), T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB8 + (0xC*num)) & 0x7FFFFFFF);
		return;
	}
	
	if((!(state->MMU->DMACrt[proc][num]&(1<<31)))&&(!(state->MMU->DMACrt[proc][num]&(1<<25))))
	{       /* not enabled and not to be repeated */
		state->MMU->DMAStartTime[proc][num] = 0;
		state->MMU->DMACycle[proc][num] = 0;
		//MMU->DMAing[proc][num] = FALSE;
		return;
	}
	
	
	/* word count */
	taille = (state->MMU->DMACrt[proc][num]&0xFFFF);
	
	// If we are in "Main memory display" mode just copy an entire 
	// screen (256x192 pixels). 
	//    Reference:  http://nocash.emubase.de/gbatek.htm#dsvideocaptureandmainmemorydisplaymode
	//       (under DISP_MMEM_FIFO)
	if ((state->MMU->DMAStartTime[proc][num]==4) &&		// Must be in main memory display mode
		(taille==4) &&							// Word must be 4
		(((state->MMU->DMACrt[proc][num]>>26)&1) == 1))	// Transfer mode must be 32bit wide
		taille = 256*192/2;
	
	if(state->MMU->DMAStartTime[proc][num] == 5)
		taille *= 0x80;
	
	state->MMU->DMACycle[proc][num] = taille + state->nds->cycles;
	state->MMU->DMAing[proc][num] = TRUE;
	
	DMALOG("proc %d, dma %d src %08X dst %08X start %d taille %d repeat %s %08X\r\n",
		proc, num, src, dst, state->MMU->DMAStartTime[proc][num], taille,
		(state->MMU->DMACrt[proc][num]&(1<<25))?"on":"off",state->MMU->DMACrt[proc][num]);
	
	if(!(state->MMU->DMACrt[proc][num]&(1<<25)))
		state->MMU->DMAStartTime[proc][num] = 0;
	
	// transfer
	{
		u32 i=0;
		// 32 bit or 16 bit transfer ?
		int sz = ((state->MMU->DMACrt[proc][num]>>26)&1)? 4 : 2;
		int dstinc,srcinc;
		int u=(state->MMU->DMACrt[proc][num]>>21);
		switch(u & 0x3) {
			case 0 :  dstinc =  sz; break;
			case 1 :  dstinc = -sz; break;
			case 2 :  dstinc =   0; break;
			case 3 :  dstinc =  sz; break; //reload
		}
		switch((u >> 2)&0x3) {
			case 0 :  srcinc =  sz; break;
			case 1 :  srcinc = -sz; break;
			case 2 :  srcinc =   0; break;
			case 3 :  // reserved
				return;
		}
		if ((state->MMU->DMACrt[proc][num]>>26)&1)
			for(; i < taille; ++i)
			{
				MMU_write32(state, proc, dst, MMU_read32(state, proc, src));
				dst += dstinc;
				src += srcinc;
			}
		else
			for(; i < taille; ++i)
			{
				MMU_write16(state, proc, dst, MMU_read16(state, proc, src));
				dst += dstinc;
				src += srcinc;
			}
	}
}

#ifdef MMU_ENABLE_ACL

INLINE void check_access(NDS_state *state, u32 adr, u32 access) {
	/* every other mode: sys */
	access |= 1;
	if ((state->NDS_ARM9->CPSR.val & 0x1F) == 0x10) {
		/* is user mode access */
		access ^= 1 ;
	}
	if (armcp15_isAccessAllowed((armcp15_t *)state->NDS_ARM9->coproc[15],adr,access)==FALSE) {
		state->execute = FALSE ;
	}
}
INLINE void check_access_write(NDS_State *state, u32 adr) {
	u32 access = CP15_ACCESS_WRITE;
	check_access(state, adr, access)
}

u8 FASTCALL MMU_read8_acl(NDS_state *state, u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(state, adr, access);
	return MMU_read8(state,proc,adr);
}
u16 FASTCALL MMU_read16_acl(NDS_state *state, u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(state, adr, access);
	return MMU_read16(state,proc,adr);
}
u32 FASTCALL MMU_read32_acl(NDS_state *state, u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(state, adr, access);
	return MMU_read32(state,proc,adr);
}

void FASTCALL MMU_write8_acl(NDS_state *state, u32 proc, u32 adr, u8 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(state,adr);
	MMU_write8(state,proc,adr,val);
}
void FASTCALL MMU_write16_acl(NDS_state *state, u32 proc, u32 adr, u16 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(state,adr);
	MMU_write16(state,proc,adr,val) ;
}
void FASTCALL MMU_write32_acl(NDS_state *state, u32 proc, u32 adr, u32 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(state,adr);
	MMU_write32(state,proc,adr,val) ;
}
#endif

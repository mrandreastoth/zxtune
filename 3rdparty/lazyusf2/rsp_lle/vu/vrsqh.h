/******************************************************************************\
* Authors:  Iconoclast                                                         *
* Release:  2013.11.26                                                         *
* License:  CC0 Public Domain Dedication                                       *
*                                                                              *
* To the extent possible under law, the author(s) have dedicated all copyright *
* and related and neighboring rights to this software to the public domain     *
* worldwide. This software is distributed without any warranty.                *
*                                                                              *
* You should have received a copy of the CC0 Public Domain Dedication along    *
* with this software.                                                          *
* If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.             *
\******************************************************************************/
#include "vu.h"
#include "divrom.h"

static void VRSQH(struct rsp_core* sp, int vd, int de, int vt, int e)
{
    sp->DivIn = sp->VR[vt][e & 07] << 16;
    SHUFFLE_VECTOR(VACC_L, sp->VR[vt], e);
    sp->VR[vd][de &= 07] = sp->DivOut >> 16;
    sp->DPH = SP_DIV_PRECISION_DOUBLE;
    return;
}

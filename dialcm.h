/*
 * Copyright 2016 Vladimir Korol <vovabox@mail.ru>
 *
 * midi4lcm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * midi4lcm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with midi4lcm. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DIA_LCM_H
#define DIA_LCM_H

#include <stdint.h>
#include "ikbusframe.h"

#define LIGHT_UNIT_SIZE 25
#define LIGHT_BIT_SHIFT(n,m) (m[(n)/8] = 0x80 >> ((n)%8)) /* Set bit in array */

typedef uint32_t light_mask_t;

typedef struct {
    const char*     name;
    light_mask_t   bitmask;
}light_unit_t;

/* Diagnostic messages for BMW E39 1996*/
light_unit_t light_units[LIGHT_UNIT_SIZE] = {
    /* FRONT */
        /* right */
    {"fog light front right",           0x00004000},
    {"turn signal front right",         0x00400000},
    {"parking light front right",       0x00200000},
    {"low beam light front right",      0x00000020},
    {"high beam light front right",     0x00002000},
        /* left */
    {"high beam light front left",      0x00001000},
    {"low beam light front left",       0x00000040},
    {"parking light front left",        0x00000100},
    {"turn signal front left",          0x40000000},
    {"fog light front left",            0x00000400},

    /* REAR */
        /* right */
    {"brake light back right",          0},
    {"backup light back right",         0},
    {"fog light back right",            0},
    {"tail light outside back right",   0},
    {"tail light inside back right",    0},
    {"turn signal back right",          0},
    {"registration plate light right",  0},
        /* center */
    {"brake light back center",         0},
        /* left */
    {"registration plate light left",   0},
    {"turn signal back left",           0},
    {"tail light inside back left",     0},
    {"tail light outside back left",    0},
    {"fog light back left",             3},
    {"backup light back left",          0},
    {"brake light back left",           0},
};

#endif /* DIA_LCM_H */


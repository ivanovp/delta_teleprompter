/**
 * @file        script.h
 * @brief       Script definitions
 * @author      Copyright (C) Peter Ivanov, 2021
 *
 * Created      2021-02-17 19:48:53
 * Last modify: 2021-02-17 19:51:49 ivanovp {Time-stamp}
 * Licence:     GPL
 */

#ifndef INCLUDE_SCRIPT_H
#define INCLUDE_SCRIPT_H

#include <stdint.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#include "common.h"
#include "linkedlist.h"

typedef struct
{
    TTF_Font *ttf_font;
    SDL_Color sdl_text_color;
    linkedList_t wrappedScriptList;
    uint32_t wrappedScriptHeightPx;
} wrappedScript_t;

#endif /* INCLUDE_SCRIPT_H */


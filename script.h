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
    TTF_Font        *ttf_font;
    linkedList_t    wrappedScriptList;      /* Linked list of wrapped lines */
    uint16_t        wrappedScriptHeightPx;  /* Height of one line */
    uint16_t        heightOffsetPx;         /* Offset inside on line. Range: 0 .. wrappedScriptHeightPx - 1 */
    uint16_t        linePerScreen;          /* Count of lines on screen */
    bool_t          isEnd;                  /* TRUE: End of script reached */
    uint16_t        maxWidthPx;
    uint16_t        maxHeightPx;
    config_t      * config;                 /* Actual configuration */
} wrappedScript_t;

#endif /* INCLUDE_SCRIPT_H */


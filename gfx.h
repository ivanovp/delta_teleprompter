/**
 * @file        game_gfx.c
 * @brief       Header of tetris-like game's common functions
 * @author      (C) Peter Ivanov, 2013, 2014
 * 
 * Created:     2013-12-23 11:29:32
 * Last modify: 2014-01-15 09:21:58 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef INCLUDE_GFX_H
#define INCLUDE_GFX_H

#include "common.h"

#include <SDL/SDL.h>

#define DEFAULT_BG_COLOR        gfx_color_rgb (0x00, 0x00, 0x00)    /* Black */
#define DEFAULT_TEXT_COLOR      gfx_color_rgb (0xFF, 0xFF, 0xFF)    /* White */

#define VIDEO_SIZE_X_PX         1024
#define VIDEO_SIZE_Y_PX         768
#define VIDEO_DEPTH_BIT         32

#define FONT_SMALL_SIZE_X_PX    8
#define FONT_SMALL_SIZE_Y_PX    12

#define FONT_NORMAL_SIZE_X_PX   8
#define FONT_NORMAL_SIZE_Y_PX   12

#define TEXT_X_0        (0)
#define TEXT_Y_0        (0)
#define TEXT_X(x)       (TEXT_X_0 + FONT_SMALL_SIZE_X_PX * (x))
#define TEXT_Y(y)       (TEXT_Y_0 + (FONT_SMALL_SIZE_Y_PX) * (y))
#define TEXT_XN(x)      (TEXT_X_0 + FONT_NORMAL_SIZE_X_PX * (x))
#define TEXT_YN(y)      (TEXT_Y_0 + (FONT_NORMAL_SIZE_Y_PX) * (y))

#ifndef DATA_DIR
#define DATA_DIR                "."
#endif
#define GFX_DIR                 DATA_DIR "/gfx/"
#define BACKGROUND_PNG          GFX_DIR "bg.png"

#define gfx_color_rgb(r,g,b)                    ( ( r << 24 ) | ( g << 16 ) | ( b << 8 ) | 0xFF )
#define gfx_line_draw(x1, y1, x2, y2, color)    lineColor(screen, x1, y1, x2, y2, color)
#define gfx_font_print(x,y,s)                   stringColor(screen, x, y, s, gfx_color_rgb(0xFF, 0xFF, 0xFF));
#define gfx_font_print_fromright(x,y,s)         stringColor(screen, x - strlen(s) * FONT_NORMAL_SIZE_X_PX, y, s, DEFAULT_TEXT_COLOR);
#define gfx_font_print_center(y,s)              stringColor(screen, screen->w / 2 - strlen(s) / 2 * FONT_NORMAL_SIZE_X_PX, y, s, DEFAULT_TEXT_COLOR)

extern SDL_Surface* background;
extern SDL_Surface* screen;

void printCommon (void);
void drawScreen (void);
void drawInfoScreen (const char* aInfo);

#endif /* INCLUDE_GFX_H */

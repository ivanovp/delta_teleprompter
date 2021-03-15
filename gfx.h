/**
 * @file        gfx.h
 * @brief       Header of graphic functions
 * @author      (C) Peter Ivanov, 2013, 2014, 2021
 * 
 * Created:     2013-12-23 11:29:32
 * Last modify: 2021-02-16 17:51:58 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef INCLUDE_GFX_H
#define INCLUDE_GFX_H

#include <SDL/SDL.h>

#include "common.h"
#include "linkedlist.h"

#define FONT_NORMAL_SIZE_X_PX   8
#define FONT_NORMAL_SIZE_Y_PX   12

#define TEXT_X_0        (0)
#define TEXT_Y_0        (0)
#define TEXT_X(x)       (TEXT_X_0 + FONT_NORMAL_SIZE_X_PX * (x))
#define TEXT_Y(y)       (TEXT_Y_0 + (FONT_NORMAL_SIZE_Y_PX) * (y))

#ifndef DATA_DIR
#define DATA_DIR                "."
#endif
#define GFX_DIR                 DATA_DIR "/gfx/"
#define BACKGROUND_PNG          GFX_DIR "bg.png"

#define gfx_color_rgb(r,g,b)                    ( ( r << 24 ) | ( g << 16 ) | ( b << 8 ) | 0xFF )
#define gfx_line_draw(x1, y1, x2, y2, color)    lineColor(screen, x1, y1, x2, y2, color)
#define gfx_font_print(x,y,s)                   stringRGBA(screen, x, y, s, config.text_color.r, config.text_color.g, config.text_color.b, 0xFF);
#define gfx_font_print_fromright(x,y,s)         stringRGBA(screen, x - strlen(s) * FONT_NORMAL_SIZE_X_PX, y, s, config.text_color.r, config.text_color.g, config.text_color.b, 0xFF);
#define gfx_font_print_center(y,s)              stringRGBA(screen, screen->w / 2 - strlen(s) / 2 * FONT_NORMAL_SIZE_X_PX, y, s, config.text_color.r, config.text_color.g, config.text_color.b, 0xFF)

extern SDL_Surface* background;
extern SDL_Surface* alphaSurface;
extern SDL_Surface* screen;

void printCommon (void);
void drawScreen (void);
void drawInfoScreen (const char *aFmt, ...);
void drawTopInfoScreen (const char *aFmt, ...);
void drawHelpScreen(void);

#endif /* INCLUDE_GFX_H */

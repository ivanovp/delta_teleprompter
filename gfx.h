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

#if USE_INTERNAL_SDL_FONT
#define FONT_NORMAL_SIZE_X_PX   8
#define FONT_NORMAL_SIZE_Y_PX   12
#else
extern int ttf_font_size_x;
extern int ttf_font_size_y;
#define FONT_NORMAL_SIZE_X_PX   (ttf_font_size_x)
#define FONT_NORMAL_SIZE_Y_PX   (ttf_font_size_y)
#endif

#define TEXT_X_0                (0)
#define TEXT_Y_0                (0)
#define TEXT_X(x)               (TEXT_X_0 + FONT_NORMAL_SIZE_X_PX * (x))
#define TEXT_Y(y)               (TEXT_Y_0 + (FONT_NORMAL_SIZE_Y_PX) * (y))
#define TEXT_Y_CENTER(y)        (screen->h / 2 + (FONT_NORMAL_SIZE_Y_PX) * (y))

#if USE_INTERNAL_SDL_FONT
#define gfx_font_print(x,y,s)                   stringRGBA(screen, x, y, s, config.text_color.r, config.text_color.g, config.text_color.b, 0xFF)
#define gfx_font_print_fromright(x,y,s)         stringRGBA(screen, x - strlen(s) * FONT_NORMAL_SIZE_X_PX, y, s, config.text_color.r, config.text_color.g, config.text_color.b, 0xFF)
#define gfx_font_print_center(y,s)              stringRGBA(screen, screen->w / 2 - strlen(s) / 2 * FONT_NORMAL_SIZE_X_PX, y, s, config.text_color.r, config.text_color.g, config.text_color.b, 0xFF)
#else
void gfx_font_print_center(int y, const char * str);
#endif

#define gfx_line_draw(x1, y1, x2, y2)                lineRGBA(screen, x1, y1, x2, y2, config.text_color.r, config.text_color.g, config.text_color.b, 0xFF)

extern SDL_Surface* background;
extern SDL_Surface* alphaSurface;
extern SDL_Surface* screen;

void printCommon (void);
void drawScreen (void);
void drawInfoScreen (const char *aFmt, ...);
void drawTopInfoScreen (const char *aFmt, ...);
void drawHelpScreen(void);

#endif /* INCLUDE_GFX_H */

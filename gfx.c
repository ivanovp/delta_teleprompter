/**
 * @file        gfx.c
 * @brief       Graphic functions
 * @author      (C) Peter Ivanov, 2013, 2014, 2021
 * 
 * Created:     2013-12-23 11:29:32
 * Last modify: 2021-02-15 18:52:14 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>

#include "common.h"
#include "gfx.h"
#include "linkedlist.h"
#include "script.h"

#define DEFAULT_INFO_TEXT_TIMER     200 // display text for 2 seconds

uint32_t     infoTextTimer = DEFAULT_INFO_TEXT_TIMER;
char infoText[512] = "Telepromter started";
const char* helpText[] =
{
    "This is Delta Teleprompter.",
    "",
    "Copyright (C) Peter Ivanov <ivanovp@gmail.com>, 2021",
    "Homepage: http://dev.ivanov.eu",
    "Licence: GPLv3",
    "",
    "This program comes with ABSOLUTELY NO WARRANTY;",
    "for details see LICENSE. This is free software,",
    "and you are welcome to redistribute it under",
    "certain conditions; see LICENSE for details.",
    "",
    "During play you can use these buttons:",
    "Enter/Space: Pause/play text",
    "Escape: Exit",
    "Up/Down: Scroll text up/down",
    "Left/Right: Change speed of scrolling",
    "+/-: Increase/decrease font size",
    "F1: This help",
    "F2: Toggle align of text (center/left)",
    "F3/F4: Descrease/increase scroll line",
    "count when pressed up/down",
    "F5/F6: Descrease/increase text width",
    "F7/F8: Descrease/increase text height",
    "F11: Toggle fullscreen",
    ""
    "Press 'Enter' to start teleprompter."
};

extern wrappedScript_t wrappedScript;

SDL_Surface * loadImage(const char* filename)
{
  SDL_Surface* loadedImage = NULL;
  SDL_Surface* optimizedImage = NULL;

  printf("Loading %s...", filename);
  loadedImage = IMG_Load (filename);

  // Check if image loaded
  if (loadedImage != NULL)
  {
      // Create an optimized image
      optimizedImage = SDL_DisplayFormat (loadedImage);

      // Free the old image
      SDL_FreeSurface (loadedImage);

      if (optimizedImage != NULL)
      {
          // Map the color key
          Uint32 colorkey = SDL_MapRGB (optimizedImage->format, 0xFF, 0, 0xFF);

          // Set all pixels of color R 0xFF, G 0, B 0xFF to be transparent
          SDL_SetColorKey (optimizedImage, SDL_SRCCOLORKEY, colorkey);

          printf("Done\n");
      }
      else
      {
        printf("Error: cannot optimize image!\n");
      }
  }
  else
  {
    printf("Error: cannot load image!\n");
  }

  // Return the optimized image
  return optimizedImage;
}

void printCommon (void)
{
    SDL_Rect sdl_rect;
    Uint32 background_color;
    char s[64];
    background_color = SDL_MapRGB(screen->format, config.background_color.r, config.background_color.g, config.background_color.b);

    if (infoTextTimer)
    {
        sdl_rect.x = 0;
        sdl_rect.y = 0;
        sdl_rect.w = config.video_size_x_px;
        sdl_rect.h = TEXT_Y(2) + FONT_NORMAL_SIZE_Y_PX / 2;
        SDL_FillRect(screen, &sdl_rect, background_color);
        gfx_line_draw (0, TEXT_Y(2), config.video_size_x_px, TEXT_Y(2));

        gfx_font_print_center(TEXT_Y(1), infoText);
        infoTextTimer--;
    }
    if (TELEPROMPTER_IS_FINISHED())
    {
        gfx_font_print_center(TEXT_Y_CENTER(0), "Press 'ENTER' to replay,");
        gfx_font_print_center(TEXT_Y_CENTER(1), "'ESCAPE' to quit...");
    }
    else if (TELEPROMPTER_IS_PAUSED())
    {
        sdl_rect.x = 0;
        sdl_rect.y = 0;
        sdl_rect.w = config.video_size_x_px;
        sdl_rect.h = TEXT_Y(4) + FONT_NORMAL_SIZE_Y_PX / 2;
        SDL_FillRect(screen, &sdl_rect, background_color);

        gfx_line_draw (0, TEXT_Y(5), config.video_size_x_px, TEXT_Y(5));

        gfx_font_print_center(TEXT_Y(1), "** PAUSED **");
        snprintf (s, sizeof (s), "Delta Teleprompter v%i.%i.%i", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
        gfx_font_print_center(TEXT_Y(2), s);
        gfx_font_print_center(TEXT_Y(3), "Copyright (C) Peter Ivanov");
        gfx_font_print_center(TEXT_Y(4), "<ivanovp@gmail.com>, 2021");
    }
    else
    {
    }
}

void drawScript(wrappedScript_t * aWrappedScript)
{
    char                * text = NULL;
    SDL_Rect              sdl_rect;
    SDL_Surface         * sdl_text;
    linkedList_t        * wrappedScriptList = &( aWrappedScript->wrappedScriptList );
    TTF_Font            * ttfFont = aWrappedScript->ttf_font;
    linkedListElement_t * linkedListElement = wrappedScriptList->actual;
    config_t            * config = aWrappedScript->config;
    SDL_Color             sdlTextColor = config->text_color;
    Sint16                y_hide_px = (config->video_size_y_px - aWrappedScript->maxHeightPx) / 2;
    Sint16                x = (config->video_size_x_px - aWrappedScript->maxWidthPx) / 2;
    Sint16                y = -(aWrappedScript->heightOffsetPx);
    Uint32                background_color;

    sdl_rect.x = x;
    sdl_rect.y = y;

//    printf("%s start\n", __FUNCTION__);
    /* Display lines of script until reaching end of script or end of display */
    while (linkedListElement && sdl_rect.y < config->video_size_y_px)
    {
        text = (char*)linkedListElement->item;
//        printf("[%s]\n", text);

        sdl_text = TTF_RenderUTF8_Blended(ttfFont, text, sdlTextColor);
        if (sdl_text == NULL)
        {
            printf("TTF_RenderUTF8_Blended() Failed: %s\n", TTF_GetError());
            break;
        }

        if (config->align_center)
        {
            sdl_rect.x = config->video_size_x_px / 2 - sdl_text->clip_rect.w / 2;
        }

        sdl_rect.w = sdl_text->clip_rect.w;
        sdl_rect.h = sdl_text->clip_rect.h;

        // Apply the text to the display
        if (SDL_BlitSurface(sdl_text, NULL, screen, &sdl_rect) != 0)
        {
            printf("SDL_BlitSurface() Failed: %s\n", SDL_GetError());
        }

        SDL_FreeSurface(sdl_text);

        /* Advance to next gfx_line_draw of script */
        y += aWrappedScript->wrappedScriptHeightPx;
        sdl_rect.y = y;
        linkedListElement = linkedListElement->next;
    }

    background_color = SDL_MapRGB(screen->format, config->background_color.r, config->background_color.g, config->background_color.b);

    // TODO add fading
    sdl_rect.x = 0;
    sdl_rect.y = 0;
    sdl_rect.w = config->video_size_x_px;
    sdl_rect.h = y_hide_px;
    SDL_FillRect(screen, &sdl_rect, background_color);

    sdl_rect.x = 0;
    sdl_rect.y = config->video_size_y_px - y_hide_px;
    sdl_rect.w = config->video_size_x_px;
    sdl_rect.h = y_hide_px;
    SDL_FillRect(screen, &sdl_rect, background_color);

//    printf("%s end\n", __FUNCTION__);
}

void drawScreen (void)
{
    if (redrawScreen)
    {
        // Restore background
        SDL_BlitSurface(background, NULL, screen, NULL);

        drawScript(&wrappedScript);
        printCommon ();

        SDL_Flip( screen );
    }
}

/**
 * Prints message in the center of screen.
 * Printf function which prints to middle of screen.
 * Note: display shall be initialized to use this function!
 *
 * Example:
<pre>
drawInfoScreen ("Number: %02i\r\n", number);
</pre>
 *
 * @param fmt Printf format string. Example: "Value: %02i\r\n"
 */
void drawInfoScreen (const char *aFmt, ...)
{
    static va_list valist;
    char buf[128];
    int  delayCntr = 500;

    va_start (valist, aFmt);
    vsnprintf (buf, sizeof (buf), aFmt, valist);
    va_end (valist);
    SDL_BlitSurface(background, NULL, screen, NULL);
    gfx_font_print_center(screen->h / 2, buf);
    SDL_Flip(screen);
    while (delayCntr-- && !eventHandler())
    {
        SDL_Delay(1);
    }
}

/**
 * Prints message in the top of screen.
 * Printf function which prints to middle of screen.
 * Note: display shall be initialized to use this function!
 *
 * Example:
<pre>
drawInfoScreen ("Number: %02i\r\n", number);
</pre>
 *
 * @param fmt Printf format string. Example: "Value: %02i\r\n"
 */
void drawTopInfoScreen (const char *aFmt, ...)
{
    static va_list valist;

    va_start (valist, aFmt);
    vsnprintf (infoText, sizeof (infoText), aFmt, valist);
    va_end (valist);
    infoTextTimer = DEFAULT_INFO_TEXT_TIMER;
}

/**
 * @brief drawHelp
 * Print help text in the center of the screen.
 */
void drawHelpScreen(void)
{
    uint8_t i;

    SDL_BlitSurface(background, NULL, screen, NULL);
    uint16_t y_center = config.video_size_y_px / FONT_SMALL_SIZE_Y_PX / 2 - (sizeof(helpText) / sizeof(helpText[0]) / 2);
    for (i = 0; i < sizeof(helpText) / sizeof(helpText[0]); i++)
    {
        gfx_font_small_print_center(TEXT_SMALL_Y(y_center + i), (char*) helpText[i]);
    }
    SDL_Flip(screen);
}

#if USE_INTERNAL_SDL_FONT == 0
void gfx_font_print_center(int y, const char *str)
{
    SDL_Surface   * sdl_text;
    SDL_Rect        sdl_rect;
    SDL_Color       sdlTextColor = config.text_color;
    int             len = strlen(str);

    if (len)
    {
        sdl_text = TTF_RenderUTF8_Blended(ttf_font_monospace, str, sdlTextColor);
        if (sdl_text == NULL)
        {
            errorprintf("TTF_RenderUTF8_Blended() Failed: %s\n", TTF_GetError());
        }

        sdl_rect.x = screen->w / 2 - len / 2 * FONT_NORMAL_SIZE_X_PX;
        sdl_rect.y = y;
        sdl_rect.w = sdl_text->clip_rect.w;
        sdl_rect.h = sdl_text->clip_rect.h;

        // Apply the text to the display
        if (SDL_BlitSurface(sdl_text, NULL, screen, &sdl_rect) != 0)
        {
            printf("SDL_BlitSurface() Failed: %s\n", SDL_GetError());
        }

        SDL_FreeSurface(sdl_text);
    }
}

void gfx_font_small_print_center(int y, const char *str)
{
    SDL_Surface   * sdl_text;
    SDL_Rect        sdl_rect;
    SDL_Color       sdlTextColor = config.text_color;
    int             len = strlen(str);

    if (len)
    {
        sdl_text = TTF_RenderUTF8_Blended(ttf_font_small_monospace, str, sdlTextColor);
        if (sdl_text == NULL)
        {
            errorprintf("TTF_RenderUTF8_Blended() Failed: %s\n", TTF_GetError());
        }

        sdl_rect.x = screen->w / 2 - len / 2 * FONT_SMALL_SIZE_X_PX;
        sdl_rect.y = y;
        sdl_rect.w = sdl_text->clip_rect.w;
        sdl_rect.h = sdl_text->clip_rect.h;

        // Apply the text to the display
        if (SDL_BlitSurface(sdl_text, NULL, screen, &sdl_rect) != 0)
        {
            printf("SDL_BlitSurface() Failed: %s\n", SDL_GetError());
        }

        SDL_FreeSurface(sdl_text);
    }
}
#endif

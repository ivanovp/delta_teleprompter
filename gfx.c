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
    char s[64];

#if 0
    gfx_line_draw (MAP_SIZE_X_PX + 1, 0,
                   MAP_SIZE_X_PX + 1, MAP_SIZE_Y_PX,
                   gfx_color_rgb (0xFF, 0xFF, 0xFF));
    sprintf (s, "Score: ");
    gfx_font_print(TEXT_X(0), TEXT_YN(0), s);
    sprintf (s, "Level: ");
    gfx_font_print(TEXT_X(0), TEXT_YN(1), s);
#endif
    if (TELEPROMPTER_IS_FINISHED())
    {
        gfx_font_print_center(TEXT_YN(11), "Press 'ENTER' to replay,");
        gfx_font_print_center(TEXT_YN(12), "'ESCAPE' to quit...");
    }
    else if (TELEPROMPTER_IS_PAUSED())
    {
        gfx_font_print(TEXT_X(0), TEXT_Y(0), "** PAUSED **");
        snprintf (s, sizeof (s), "Delta Teleprompter v%i.%i.%i", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
        gfx_font_print(TEXT_X(0), TEXT_Y(1), s);
        gfx_font_print(TEXT_X(0), TEXT_Y(2), "Copyright (C) Peter Ivanov <ivanovp@gmail.com>, 2021");
    }
    else
    {
    }
#if 0
    /* Small debug */
    snprintf (s, sizeof (s), "C");
    gfx_font_print (TEXT_X_0,
                    (screen->h - FONT_SMALL_SIZE_Y_PX - 4),
                    gameFontSmall, s);
    snprintf (s, sizeof (s), "v%lu.%lu.%lu", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
    gfx_font_print_fromright ((screen->w - 4),
                              (screen->h - FONT_SMALL_SIZE_Y_PX - 4), s);
#endif
}

void drawScript(wrappedScript_t * aWrappedScript)
{
    char                * text = NULL;
    SDL_Rect              sdl_rect;
    SDL_Surface         * sdl_text;
    linkedList_t        * wrappedScriptList = &( aWrappedScript->wrappedScriptList );
    TTF_Font            * ttfFont = aWrappedScript->ttf_font;
    SDL_Color             sdlTextColor = aWrappedScript->sdl_text_color;
    linkedListElement_t * linkedListElement = wrappedScriptList->actual;
    config_t            * config = aWrappedScript->config;
    Sint16                y_hide_px = ((float)config->video_size_y_px * 100.0f / ( 100.0f - config->text_height_percent ) ) / 2;
    Sint16                y = -(aWrappedScript->heightOffsetPx);

    sdl_rect.x = 0;
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
            printf("TTF_RenderText_Solid() Failed: %s\n", TTF_GetError());
            break;
        }

        sdl_rect.w = sdl_text->clip_rect.w;
        sdl_rect.h = sdl_text->clip_rect.h;

        // Apply the text to the display
        if (SDL_BlitSurface(sdl_text, NULL, screen, &sdl_rect) != 0)
        {
            printf("SDL_BlitSurface() Failed: %s\n", SDL_GetError());
        }

        SDL_FreeSurface(sdl_text);

        /* Advance to next line of script */
        y += aWrappedScript->wrappedScriptHeightPx;
        sdl_rect.y = y;
        linkedListElement = linkedListElement->next;
    }
#if 0
    sdl_rect.x = 0;
    sdl_rect.y = 0;
    sdl_rect.w = config->video_size_x_px;
    sdl_rect.h = y_hide_px;
    SDL_FillRect(screen, &sdl_rect, config->background_color);

    sdl_rect.x = 0;
    sdl_rect.y = config->video_size_y_px - y_hide_px - 1;
    sdl_rect.w = config->video_size_x_px;
    sdl_rect.h = y_hide_px;
    SDL_FillRect(screen, &sdl_rect, config->background_color);
#endif
//    printf("%s end\n", __FUNCTION__);
}

void drawScreen (void)
{
    // Restore background
    SDL_BlitSurface( background, NULL, screen, NULL );

    printCommon ();
    drawScript(&wrappedScript);

    SDL_Flip( screen );
}

/**
 * Prints message in the center of screen.
 * Note: display shall be initialized to use this function!
 *
 * @param aInfo Text to print.
 */
void drawInfoScreen (const char* aInfo)
{
    SDL_BlitSurface( background, NULL, screen, NULL );
    gfx_font_print_center (screen->h / 2, aInfo);
    SDL_Flip( screen );
}

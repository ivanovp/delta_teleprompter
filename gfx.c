/**
 * @file        game_gfx.c
 * @brief       Tetris-like game's common functions
 * @author      (C) Peter Ivanov, 2013, 2014
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
#include <SDL/SDL_gfxPrimitives.h>

#include "common.h"
#include "gfx.h"

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
        gfx_font_print (TEXT_X_0, TEXT_YN(8), "** GAME **");
        gfx_font_print (TEXT_X_0, TEXT_YN(9), "** OVER **");
        gfx_font_print (TEXT_X_0, TEXT_YN(10), "Press Enter");
        gfx_font_print (TEXT_X_0, TEXT_YN(11), "to replay,");
        gfx_font_print (TEXT_X_0, TEXT_YN(12), "Escape to quit...");
    }
    else if (TELEPROMPTER_IS_PAUSED())
    {
        gfx_font_print(TEXT_X_0, TEXT_YN(8), "** PAUSED **");
        snprintf (s, sizeof (s), "Teleprompter v%i.%i.%i", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
        gfx_font_print(TEXT_X_0, TEXT_Y(11), s);
        gfx_font_print(TEXT_X_0, TEXT_Y(12), "Copyright (C)");
        gfx_font_print(TEXT_X_0, TEXT_Y(13), "Peter Ivanov,");
        gfx_font_print(TEXT_X_0, TEXT_Y(14), "2021");
        gfx_font_print(TEXT_X_0, TEXT_Y(15), "ivanovp@gmail.com");
        gfx_font_print(TEXT_X_0, TEXT_Y(16), "http://dev.ivanov.eu");
        gfx_font_print(TEXT_X_0, TEXT_Y(17), "Licence: GPLv3");
        gfx_font_print(TEXT_X_0, TEXT_Y(18), "ABSOLUTELY");
        gfx_font_print(TEXT_X_0, TEXT_Y(19), "NO WARRANTY!");
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

void drawScreen (void)
{
    // Restore background
    SDL_BlitSurface( background, NULL, screen, NULL );

    printCommon ();
    if (TELEPROMPTER_IS_PAUSED() || TELEPROMPTER_IS_FINISHED())
    {
    }

    SDL_Flip( screen );
}

/**
 * Prints message in the center of screen.
 * Note: gameDisplay shall be initialized to use this function!
 *
 * @param aInfo Text to print.
 */
void drawInfoScreen (const char* aInfo)
{
    SDL_BlitSurface( background, NULL, screen, NULL );
    gfx_font_print_center (screen->h / 2, aInfo);
    SDL_Flip( screen );
}

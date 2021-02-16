/**
 * @file        main.c
 * @brief       Sometris main
 * @author      Copyright (C) Peter Ivanov, 2013, 2014
 *
 * Created      2013-12-30 11:48:53
 * Last modify: 2021-02-15 19:01:21 ivanovp {Time-stamp}
 * Licence:     GPL
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_timer.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>

#include "common.h"
#include "gfx.h"

#define CONFIG_DIR              "/.delta_teleprompter"
#define CONFIG_FILENAME         CONFIG_DIR "/teleprompter.bin"

#define MAX_KEYS                6
#define KEY_UP                  0
#define KEY_DOWN                1
#define KEY_LEFT                2
#define KEY_RIGHT               3
#define KEY_ENTER               4
#define KEY_SPACE               5

#define FAST_REPEAT_TICK        150
#define NORMAL_REPEAT_TICK      250

typedef struct
{
  bool_t pressed;
  bool_t changed;
  uint32_t pressTick;
  uint32_t repeatTick;
} mykey_t;

const char* info[] =
{
    "This is Delta Teleprompter.",
    "",
    "Copyright (C) Peter Ivanov <ivanovp@gmail.com>, 2021",
    "",
    "This program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.",
    "This is free software, and you are welcome to redistribute it under certain",
    "conditions; see LICENSE for details.",
    "",
    "During play you can use these buttons:",
    "ENTER: Pause text",
    "ESCAPE: Exit",
    "Left/right/up/down: Move",
    ""
    "Press 'ENTER' to start teleprompter."
};

const char *homeDir;

/* Teleprompter related */
uint32_t     teleprompterTimer = 0;
bool_t       teleprompterRunning   = TRUE;
main_state_machine_t main_state_machine = STATE_undefined;

char         scriptFilePath[FSYS_FILENAME_MAX];

mykey_t      keys[MAX_KEYS] = { 0 };

#define leftPressed   keys[KEY_LEFT].pressed
#define rightPressed  keys[KEY_RIGHT].pressed
#define downPressed   keys[KEY_DOWN].pressed
#define upPressed     keys[KEY_UP].pressed
#define enterPressed  keys[KEY_ENTER].pressed
#define spacePressed  keys[KEY_SPACE].pressed

#define leftChanged   keys[KEY_LEFT].changed
#define rightChanged  keys[KEY_RIGHT].changed
#define downChanged   keys[KEY_DOWN].changed
#define upChanged     keys[KEY_UP].changed
#define enterChanged  keys[KEY_ENTER].changed
#define spaceChanged  keys[KEY_SPACE].changed

/* Default configuration, could be overwritten by loadConfig() */
config_t config =
{
    .version = 1,
    .script_file_path = { 0 }
};

// The images
SDL_Surface* background = NULL;
SDL_Surface* screen = NULL;
bool_t textInputIsStarted = FALSE;
char * text = NULL;
uint32_t textLength = 0;

/**
 * Set up default configuration and load if configuration file exists.
 */
void loadConfig (void)
{
    FILE* configFile;
    config_t configTemp;
    char path[256];

    drawInfoScreen ("Loading configuration...");

    strncpy(path, homeDir, sizeof(path));
    strncat(path, CONFIG_FILENAME, sizeof(path));
    printf("%s: %s\n", __FUNCTION__, path);
    configFile = fopen (path, "rb");
    if (configFile)
    {
        size_t size;
        size = fread (&configTemp, 1, sizeof (configTemp), configFile);
        if (size == sizeof (configTemp) && configTemp.version == config.version)
        {
            /* Configuration is OK, copy it */
            memcpy (&config, &configTemp, sizeof (config));
        }
        fclose (configFile);
    }
}

/**
 * Save current configuration.
 */
bool_t saveConfig (void)
{
    bool_t ok = FALSE;
    FILE* configFile;
    char path[256];

    drawInfoScreen ("Saving configuration...");

    strncpy (config.script_file_path, scriptFilePath, sizeof (config.script_file_path));
    strncpy(path, homeDir, sizeof(path));
    strncat(path, CONFIG_FILENAME, sizeof(path));
    printf("%s: %s\n", __FUNCTION__, path);
    configFile = fopen (path, "wb");
    if (configFile)
    {
        size_t size;
        size = fwrite (&config, 1, sizeof (config), configFile);
        if (size == sizeof (config))
        {
            ok = TRUE;
        }
        fclose (configFile);
    }
    else
    {
      drawInfoScreen("Cannot save configuration!");
      SDL_Delay(500);
    }

    return ok;
}

/**
 * @brief init
 * Initialize teleprompter.
 *
 * @return TRUE: if successfully initialized. FALSE: error occurred.
 */
bool_t init (void)
{
    char path[256];

    srand(time(NULL));

    homeDir = getenv ("HOME");
    if (homeDir == NULL)
    {
      homeDir = getpwuid (getuid ())->pw_dir;
    }
    strncpy(path, homeDir, sizeof(path));
    strncat(path, CONFIG_DIR, sizeof(path));
    printf("%s mkdir: %s\r\n", __FUNCTION__, path);
    mkdir(path, 0755);

    SDL_Init(SDL_INIT_EVERYTHING);

    // Set up screen
    screen = SDL_SetVideoMode(VIDEO_SIZE_X_PX, VIDEO_SIZE_Y_PX, VIDEO_DEPTH_BIT, SDL_SWSURFACE
//                              | SDL_FULLSCREEN
                              );

    // Load background image
    background = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEO_SIZE_X_PX, VIDEO_SIZE_Y_PX, VIDEO_DEPTH_BIT, 0, 0, 0, 0);

    //Apply image to screen
    SDL_BlitSurface( background, NULL, screen, NULL );

#if 1
    // Initialize SDL_ttf library
    if (TTF_Init() != 0)
    {
        printf("TTF_Init() Failed: %s\n", TTF_GetError());
        SDL_Quit();
        exit(1);
    }

    // Load a font
    TTF_Font *font;
    font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 24);
    if (font == NULL)
    {
        printf("TTF_OpenFont() Failed: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        exit(1);
    }

    // Write text to surface
    SDL_Surface *text;
    SDL_Color text_color = {0xff, 0xff, 0xff, 0};
    text = TTF_RenderText_Blended(font,
                                "A journey of a thousand miles begins with a single step.",
                                text_color);

    if (text == NULL)
    {
        printf("TTF_RenderText_Solid() Failed: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        exit(1);
    }

    SDL_Rect rect;
    rect.x = 10;
    rect.y = 450;
    rect.w = text->clip_rect.w;
    rect.h = text->clip_rect.h;

    // Apply the text to the display
    if (SDL_BlitSurface(text, NULL, screen, &rect) != 0)
    {
        printf("SDL_BlitSurface() Failed: %s\n", SDL_GetError());
    }

    //Update Screen
    SDL_Flip( screen );
    SDL_Delay(2000);
#endif

    keys[KEY_UP].repeatTick = NORMAL_REPEAT_TICK;
    keys[KEY_DOWN].repeatTick = FAST_REPEAT_TICK;
    keys[KEY_LEFT].repeatTick = FAST_REPEAT_TICK;
    keys[KEY_RIGHT].repeatTick = FAST_REPEAT_TICK;
    keys[KEY_ENTER].repeatTick = NORMAL_REPEAT_TICK;
    keys[KEY_SPACE].repeatTick = NORMAL_REPEAT_TICK;

    loadConfig ();

    return TRUE;
}

/**
 * @brief handleMovement
 * Handle button presses and move figure according to that.
 */
void handleMovement (void)
{
    if (rightPressed && rightChanged)
    {
        /* Right */
    }
    if (leftPressed && leftChanged)
    {
        /* Left */
    }
    if (downPressed && downChanged)
    {
        /* Down */
    }
    if (upPressed && upChanged)
    {
    }
    if (SDL_GetTicks() >= teleprompterTimer)
    {
        /* Automatic fall */
        int16_t ticks;

        /* Delay time = 0.8 sec - level * 0.1 sec */
        ticks = OS_TICKS_PER_SEC * 8 / 10 - 1 * OS_TICKS_PER_SEC / 10;
        if (ticks < OS_TICKS_PER_SEC / 10) /* less than 0.1 sec */
        {
            ticks = OS_TICKS_PER_SEC / 10;
        }
        teleprompterTimer = SDL_GetTicks() + ticks;
    }
}

/**
 * @brief handle_main_state_machine
 * Check inputs and change state machine if it is necessary.
 * @return TRUE: if teleprompter shall be started again.
 */
bool_t handleMainStateMachine (void)
{
    bool_t replay = FALSE;
    uint8_t i;

    switch (main_state_machine)
    {
        case STATE_intro:
            if (enterPressed && enterChanged)
            {
                main_state_machine = STATE_load_script;
            }
            SDL_BlitSurface( background, NULL, screen, NULL );
            uint16_t y_center = VIDEO_SIZE_Y_PX / FONT_SMALL_SIZE_Y_PX / 2 - ( sizeof(info) / sizeof(info[0]) / 2 );
            for (i = 0; i < sizeof(info) / sizeof(info[0]); i++)
            {
                gfx_font_print_center(TEXT_Y(y_center + i), (char*) info[i]);
            }
            SDL_Flip(screen);
            break;
        case STATE_load_script:
            // TODO
            main_state_machine = STATE_running;
            break;
        case STATE_running:
            handleMovement ();
            if (enterPressed && enterChanged)
            {
                main_state_machine = STATE_paused;
            }
//            if (isGameOver ())
//            {
//                main_state_machine = STATE_end;
//            }
//            else
            {
                drawScreen ();
            }
            break;
        case STATE_paused:
            if (enterPressed && enterChanged)
            {
                main_state_machine = STATE_running;
            }
            drawScreen ();
            break;
        case STATE_end:
            if (enterPressed && enterChanged)
            {
                /* Restart teleprompter */
                replay = TRUE;
            }
            drawScreen ();
            break;
        default:
        case STATE_undefined:
            /* This should not happen */
            break;
    }

    return replay;
}

void key_pressed(uint8_t key_index, bool_t pressed)
{
    if (key_index < MAX_KEYS)
    {
        keys[key_index].changed = TRUE;
        keys[key_index].pressed = pressed;
        if (pressed)
        {
            keys[key_index].pressTick = SDL_GetTicks() + keys[key_index].repeatTick;
        }
        else
        {
            keys[key_index].pressTick = 0;
        }
    }
    else
    {
        printf("%s internal error\r\n", __FUNCTION__);
    }
}

void key_task()
{
    int i;
    SDL_Event event;

    for (i = 0; i < MAX_KEYS; i++)
    {
        keys[i].changed = FALSE;
        if (keys[i].pressed && keys[i].pressTick < SDL_GetTicks())
        {
            /* Simulate key has just pressed */
            keys[i].changed = TRUE;
            keys[i].pressTick = SDL_GetTicks() + keys[i].repeatTick;
        }
    }

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_KEYDOWN)
        {
            if (textInputIsStarted)
            {
              uint32_t len = strnlen(text, textLength);
              if (event.key.keysym.sym >= 32 && event.key.keysym.sym <= 126)
              {
                if (len < textLength - 1)
                {
                    text[len] = event.key.keysym.sym;
                    if (event.key.keysym.mod & (KMOD_RSHIFT | KMOD_LSHIFT) )
                    {
                      /* Make upper case */
                      text[len] &= 0xDF;
                    }
                    text[len + 1] = 0;
                }
              }
              if (event.key.keysym.sym == SDLK_BACKSPACE && len > 0)
              {
                text[len - 1] = 0;
              }
            }

            switch (event.key.keysym.sym)
            {
                case SDLK_UP:
                    key_pressed(KEY_UP, TRUE);
                    break;
                case SDLK_DOWN:
                    key_pressed(KEY_DOWN, TRUE);
                    break;
                case SDLK_LEFT:
                    key_pressed(KEY_LEFT, TRUE);
                    break;
                case SDLK_RIGHT:
                    key_pressed(KEY_RIGHT, TRUE);
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    key_pressed(KEY_ENTER, TRUE);
                    break;
                case SDLK_SPACE:
                    key_pressed(KEY_SPACE, TRUE);
                    break;
                case SDLK_ESCAPE:
                    teleprompterRunning = FALSE;
                    break;
                default:
                    break;
            }
        }
        else if (event.type == SDL_KEYUP)
        {
            switch (event.key.keysym.sym)
            {
                case SDLK_UP:
                    key_pressed(KEY_UP, FALSE);
                    break;
                case SDLK_DOWN:
                    key_pressed(KEY_DOWN, FALSE);
                    break;
                case SDLK_LEFT:
                    key_pressed(KEY_LEFT, FALSE);
                    break;
                case SDLK_RIGHT:
                    key_pressed(KEY_RIGHT, FALSE);
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    key_pressed(KEY_ENTER, FALSE);
                    break;
                case SDLK_SPACE:
                    key_pressed(KEY_SPACE, FALSE);
                    break;
                default:
                    break;
            }
        }
        else if (event.type == SDL_QUIT) /* If the user has Xed out the window */
        {
            /* Quit the program */
            teleprompterRunning = FALSE;
        }
    }
}

/**
 * @brief run
 * Run teleprompter.
 */
void run (void)
{
    main_state_machine = STATE_intro;

    while (teleprompterRunning)
    {
        key_task();
        handleMainStateMachine ();
        usleep( 10e3 );
    }
}

/**
 * @brief done
 * End teleprompter. Free resources.
 */
void done (void)
{
    saveConfig ();

    //Free the loaded image
    SDL_FreeSurface(background);

    //Quit SDL
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    if (init ())
    {
        run ();
        done ();
    }

    return 0;
}

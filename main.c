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
#include <errno.h>
#include <locale.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_timer.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>

#include "common.h"
#include "gfx.h"
#include "linkedlist.h"
#include "script.h"

#define CONFIG_DIR                  "/.delta_teleprompter"
#define CONFIG_FILENAME             CONFIG_DIR "/teleprompter.bin"

#define MAX_KEYS                    6
#define KEY_UP                      0
#define KEY_DOWN                    1
#define KEY_LEFT                    2
#define KEY_RIGHT                   3
#define KEY_ENTER                   4
#define KEY_SPACE                   5

#define FAST_REPEAT_TICK            150
#define NORMAL_REPEAT_TICK          250

#define DEFAULT_INTRO_TIMER         250
#define DEFAULT_LOAD_SCRIPT_TIMER   250

typedef struct
{
  bool_t pressed;
  bool_t changed;
  uint32_t pressTick;
  uint32_t repeatTick;
} mykey_t;

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

const char* info[] =
{
    "This is Delta Teleprompter.",
    "",
    "Copyright (C) Peter Ivanov <ivanovp@gmail.com>, 2021",
    "Homepage: http://dev.ivanov.eu",
    "Licence: GPLv3",
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

/* Default configuration, could be overwritten by loadConfig() */
config_t config =
{
    .version = 1,
    .script_file_path = { 0 }
};

/* Teleprompter related */
uint32_t     introTimer = DEFAULT_INTRO_TIMER;
uint32_t     loadScriptTimer = DEFAULT_LOAD_SCRIPT_TIMER;
uint32_t     teleprompterTimer = 0;
bool_t       teleprompterRunning   = TRUE;
main_state_machine_t main_state_machine = STATE_undefined;
main_state_machine_t main_state_machine_next = STATE_undefined;

char         scriptFilePath[FSYS_FILENAME_MAX] = "script.txt";

// The images
SDL_Surface* background = NULL;
SDL_Surface* screen = NULL;
bool_t textInputIsStarted = FALSE;
char * text = NULL;
uint32_t textLength = 0;
char * scriptBuffer = NULL;
wrappedScript_t wrappedScript =
{
    .ttf_font = NULL,
    .sdl_text_color = {0xff, 0xff, 0xff, 0},
    .wrappedScriptList = { 0 },
    .wrappedScriptHeightPx = 0
};

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
 * @brief loadScript Load script from file.
 *
 * @param[in]  aScriptFilePath  Script to load.
 * @param[out] aScriptBuffer    Target buffer.
 *
 * @return TRUE: if script successfully loaded. FALSE: if error occured. Error text is printed to screen.
 */
bool_t loadScript(const char * aScriptFilePath, char ** aScriptBuffer)
{
    bool_t  ok = FALSE;
    FILE  * file;
    size_t  fileSize;
    size_t  readBytes;

    drawInfoScreen("Loading script...");

    if (*aScriptBuffer)
    {
        printf("Releasing memory... ");
        free(*aScriptBuffer);
        *aScriptBuffer = NULL;
        printf("Done\n");
    }

    printf("Open file %s ... ", aScriptFilePath);
    file = fopen(aScriptFilePath, "r");
    if (file)
    {
        printf("Done.\n");
        if (fseek(file, 0, SEEK_END) == 0)
        {
            fileSize = ftell(file);
            if (fileSize)
            {
                if (fseek(file, 0, SEEK_SET) == 0)
                {
                    printf("Script size: %lu\n", fileSize);
                    printf("Allocating memory... ");
                    *aScriptBuffer = malloc(fileSize + 1); // +1 end of string
                    if (*aScriptBuffer)
                    {
                        printf("Done.\n");
                        readBytes = fread(*aScriptBuffer, 1, fileSize, file);
                        printf("%lu bytes were read\n", readBytes);
                        if (readBytes == fileSize)
                        {
                            (*aScriptBuffer)[fileSize] = 0; // end of string
                            drawInfoScreen("Script loaded.");
                            ok = TRUE;
                        }
                        else
                        {
                            printf("errno: %i\n", errno);
                            printf("strerror: %s\n", strerror(errno));
                            drawInfoScreen("ERROR: Cannot read from script file!");
                        }
                    }
                    else
                    {
                        printf("Error!\n");
                        drawInfoScreen("ERROR: Cannot allocate memory for script!");
                    }
                }
                else
                {
                    drawInfoScreen("ERROR: Cannot seek to beginning of file!");
                }
            }
            else
            {
                drawInfoScreen("ERROR: File is empty!");
            }
        }
        else
        {
            drawInfoScreen("ERROR: Cannot seek to end of file!");
        }
        printf("Closing file... ");
        if (!fclose(file))
        {
            printf("Done.\n");
        }
        else
        {
            printf("Error!\n");
        }
    }
    else
    {
        drawInfoScreen("ERROR: Cannot open script file!");
    }

    return ok;
}

bool_t wrapScript(char * aScriptBuffer, uint16_t aMaxWidthPx, wrappedScript_t * aWrappedScript)
{
    bool_t   ok = TRUE;
    uint32_t i;
    char   * start_ptr;
    char   * end_ptr;
    char   * prev_end_ptr;
    int      width_px;
    int      height_px;
    char     text[1024];
    size_t   len;

    start_ptr = aScriptBuffer;
    end_ptr = start_ptr;
    prev_end_ptr = start_ptr;

    freeLinkedList(aWrappedScript->wrappedScriptList.first);
    // Initialize iterator
    aWrappedScript->wrappedScriptList.it = &(aWrappedScript->wrappedScriptList.first);

    for (i = 0; aScriptBuffer[i] && ok; i++)
    {
        if (IS_WHITESPACE(aScriptBuffer[i]))
        {
            // Search end of white spaces
            for (; IS_WHITESPACE(aScriptBuffer[i]); i++)
            {
                // replace \n and \t with space
                // TODO interpret new line and start a new line?
                aScriptBuffer[i] = ' ';
            }

            prev_end_ptr = end_ptr;
            end_ptr = &aScriptBuffer[i];
            len = (uintptr_t)end_ptr - (uintptr_t)start_ptr;
            if (len < sizeof(text) - 1) // -1 due to end of string
            {
                strncpy(text, start_ptr, end_ptr - start_ptr);
                text[len] = 0; // end of string
//                printf("text: [%s]\n", text);
                TTF_SizeUTF8(aWrappedScript->ttf_font, text, &width_px, &height_px);
//                printf("width_px: %i height_px: %i\n", width_px, height_px);

#if 0
                //----------------------------------------------------------------- FIXME debug
                //Apply background to screen
                SDL_BlitSurface( background, NULL, screen, NULL );

                SDL_Surface *sdl_text;
                sdl_text = TTF_RenderUTF8_Blended(ttf_font, text, sdl_text_color);
                if (sdl_text == NULL)
                {
                    printf("TTF_RenderText_Solid() Failed: %s\n", TTF_GetError());
                    TTF_Quit();
                    SDL_Quit();
                    exit(1);
                }

                SDL_Rect sdl_rect;
                sdl_rect.x = 0;
                sdl_rect.y = VIDEO_SIZE_Y_PX / 2;
                sdl_rect.w = sdl_text->clip_rect.w;
                sdl_rect.h = sdl_text->clip_rect.h;

                // Apply the text to the display
                if (SDL_BlitSurface(sdl_text, NULL, screen, &sdl_rect) != 0)
                {
                    printf("SDL_BlitSurface() Failed: %s\n", SDL_GetError());
                }

                //Update Screen
                SDL_Flip( screen );
                SDL_Delay(200);

                key_task(); // FIXME debug
                if (!teleprompterRunning) return false; // FIXME debug
                //----------------------------------------------------------------- FIXME debug
#endif
                if (width_px >= aMaxWidthPx)
                {
                    ok = addScriptElement(start_ptr, prev_end_ptr, &(aWrappedScript->wrappedScriptList));
                    start_ptr = prev_end_ptr;
                }
            }
            else
            {
                printf("ERROR: Text too long!\n");
                ok = FALSE;
            }
        }
    }

    if (ok)
    {
        addScriptElement(start_ptr, &aScriptBuffer[i], &(aWrappedScript->wrappedScriptList));
        aWrappedScript->wrappedScriptList.actual = aWrappedScript->wrappedScriptList.first;
        aWrappedScript->wrappedScriptHeightPx = height_px;
    }
    else
    {
        /* Error occurred: free linked list */
        // FIXME display some text?
        freeLinkedList(aWrappedScript->wrappedScriptList.first);
    }

    return ok;
}

void printScript(linkedList_t * aWrappedScriptList)
{
    linkedListElement_t* linkedListElement = aWrappedScriptList->actual;

    printf("%s start\n", __FUNCTION__);
    while (linkedListElement)
    {
        printf("[%s]\n", (char*)linkedListElement->item);
        linkedListElement = linkedListElement->next;
    }
    printf("%s end\n", __FUNCTION__);
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

    setlocale(LC_ALL, ""); // FIXME needed?
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

    // Create background image
    background = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEO_SIZE_X_PX, VIDEO_SIZE_Y_PX, VIDEO_DEPTH_BIT, 0, 0, 0, 0);

    //Apply image to screen
    SDL_BlitSurface( background, NULL, screen, NULL );

    // Initialize SDL_ttf library
    if (TTF_Init() != 0)
    {
        printf("TTF_Init() Failed: %s\n", TTF_GetError());
        SDL_Quit();
        exit(1);
    }

    // Load a ttf_font
    wrappedScript.ttf_font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 24);
    if (wrappedScript.ttf_font == NULL)
    {
        printf("TTF_OpenFont() Failed: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        exit(1);
    }

#if 0
    // Write text to surface
    SDL_Surface *sdl_text;
    sdl_text = TTF_RenderText_Blended(ttf_font,
                                "A journey of a thousand miles begins with a single step.",
                                sdl_text_color);

    if (sdl_text == NULL)
    {
        printf("TTF_RenderText_Solid() Failed: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        exit(1);
    }

    SDL_Rect sdl_rect;
    sdl_rect.x = 10;
    sdl_rect.y = 450;
    sdl_rect.w = sdl_text->clip_rect.w;
    sdl_rect.h = sdl_text->clip_rect.h;

    // Apply the text to the display
    if (SDL_BlitSurface(sdl_text, NULL, screen, &sdl_rect) != 0)
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
 */
void handleMainStateMachine (void)
{
    uint8_t i;

    switch (main_state_machine)
    {
        case STATE_intro:
            introTimer--;
            if ((enterPressed && enterChanged) || !introTimer)
            {
                main_state_machine = STATE_load_script;
            }
            SDL_BlitSurface(background, NULL, screen, NULL);
            uint16_t y_center = VIDEO_SIZE_Y_PX / FONT_SMALL_SIZE_Y_PX / 2 - (sizeof(info) / sizeof(info[0]) / 2);
            for (i = 0; i < sizeof(info) / sizeof(info[0]); i++)
            {
                gfx_font_print_center(TEXT_Y(y_center + i), (char*) info[i]);
            }
            SDL_Flip(screen);
            break;
        case STATE_load_script:
            if (loadScript(scriptFilePath, &scriptBuffer))
            {
                if (wrapScript(scriptBuffer, VIDEO_SIZE_X_PX, &wrappedScript))
                {
//                    printScript(&wrappedScriptList);
                    /* Script successfully loaded, immediately show it */
                    main_state_machine = STATE_running;
                }
                else
                {
                    // TODO what next?
                }
            }
            else
            {
                /* Error occured, leave error message on the screen for a while */
                main_state_machine_next = STATE_end;
                main_state_machine = STATE_load_script_wait;
            }
            break;
        case STATE_load_script_wait:
            loadScriptTimer--;
            if (!loadScriptTimer)
            {
                main_state_machine = main_state_machine_next;
            }
            break;
        case STATE_running:
            handleMovement ();
            if (enterPressed && enterChanged)
            {
                main_state_machine = STATE_paused;
            }
            drawScreen ();
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
                introTimer = DEFAULT_INTRO_TIMER;
                loadScriptTimer = DEFAULT_LOAD_SCRIPT_TIMER;
                main_state_machine = STATE_load_script;
            }
            drawScreen ();
            break;
        default:
        case STATE_undefined:
            /* This should not happen */
            break;
    }
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

    if (scriptBuffer)
    {
        printf("Releasing memory... ");
        free(scriptBuffer);
        scriptBuffer = NULL;
        printf("Done\n");
    }

    if (wrappedScript.wrappedScriptList.first)
    {
        printf("Releasing linked list... ");
        freeLinkedList(wrappedScript.wrappedScriptList.first);
        printf("Done\n");
    }

    TTF_CloseFont(wrappedScript.ttf_font);

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

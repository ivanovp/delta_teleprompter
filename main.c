/**
 * @file        main.c
 * @brief       Delta Teleprompter main
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

#define KEY_UP                      0
#define KEY_DOWN                    1
#define KEY_LEFT                    2
#define KEY_RIGHT                   3
#define KEY_ENTER                   4
#define KEY_SPACE                   5
#define KEY_PLUS                    6
#define KEY_MINUS                   7
#define KEY_HOME                    8
#define KEY_END                     9
#define KEY_F1                      10
#define KEY_F2                      11
#define KEY_F3                      12
#define KEY_F4                      13
#define KEY_F5                      14
#define KEY_F6                      15
#define KEY_F7                      16
#define KEY_F8                      17
#define KEY_F9                      18
#define KEY_F10                     19
#define KEY_F11                     20
#define KEY_F12                     21
#define KEY_COUNT                   22 /* not a real key, just to count keys */

#define FAST_REPEAT_TICK            150
#define NORMAL_REPEAT_TICK          250

#define DEFAULT_INTRO_TIMER         250
#define DEFAULT_LOAD_SCRIPT_TIMER   250

#define MAX_FONT_SIZE               200
#define MIN_FONT_SIZE               6
#define FONT_SIZE_STEP              2

#define MAX_TEXT_WIDTH_PERCENT      100
#define MIN_TEXT_WIDTH_PERCENT      10
#define TEXT_WIDTH_PERCENT_STEP     5

#define MAX_TEXT_HEIGHT_PERCENT     100
#define MIN_TEXT_HEIGHT_PERCENT     10
#define TEXT_HEIGHT_PERCENT_STEP    5

typedef struct
{
  bool_t pressed;
  bool_t changed;
  uint32_t pressTick;
  uint32_t repeatTick;
} mykey_t;

mykey_t      keys[KEY_COUNT] = { 0 };

#define IS_PRESSED(key)             keys[(key)].pressed
#define IS_CHANGED(key)             keys[(key)].changed
#define IS_PRESSED_CHANGED(key)     (keys[(key)].pressed && keys[(key)].changed)

const char *homeDir;

/* Default configuration, could be overwritten by loadConfig() */
config_t config =
{
    .version = 1,
    .script_file_path = "script.txt",
    .ttf_file_path = "",
    .ttf_size = 36,
    .text_width_percent = 90,
    .text_height_percent = 90,
    .video_size_x_px = 640,
    .video_size_y_px = 480,
    .video_depth_bit = 16,
    .background_color = { 0, 0, 0, 0},      // default background color is black
    .text_color = { 0xFF, 0xFF, 0xFF, 0 },  // default text color is white
    .align_center = TRUE,
    .auto_scroll_speed = 240,               // range: 0..255
    .scroll_line_count = 5,
    .full_screen = FALSE,
};

/* Teleprompter related */
uint32_t     introTimer = DEFAULT_INTRO_TIMER;
uint32_t     loadScriptTimer = DEFAULT_LOAD_SCRIPT_TIMER;
bool_t       teleprompterRunning   = TRUE;
main_state_machine_t main_state_machine = STATE_undefined;
main_state_machine_t main_state_machine_next = STATE_undefined;

// The surfaces (the sceen itself, background, etc.)
SDL_Surface* background = NULL;
SDL_Surface* alphaSurface = NULL;
SDL_Surface* screen = NULL;
bool_t textInputIsStarted = FALSE;
char * text = NULL;
uint32_t textLength = 0;
char * scriptBuffer = NULL;
wrappedScript_t wrappedScript =
{
    .ttf_font = NULL,
    .wrappedScriptList = { 0 },
    .wrappedScriptHeightPx = 0,
    .config = &config,
};
SDL_TimerID autoScrollTimer = NULL;

/* Symbols for DejavuSans.o which is directly converted from .ttf to object using 'ld' */
extern uint8_t _binary_DejaVuSans_ttf_start[];
extern uint8_t _binary_DefavuSans_ttf_end;
extern uint8_t _binary_DejaVuSans_ttf_size;

Uint32 timerCallbackFunc(Uint32 interval, void *param)
{
    SDL_Event event;
    SDL_UserEvent userevent;

    /* This callback pushes an SDL_USEREVENT event
    into the queue, and causes our callback to be called again at the
    same interval: */

    userevent.type = SDL_USEREVENT;
    userevent.code = 0;
    userevent.data1 = NULL;
    userevent.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);
    return(interval);
}

/**
 * Set up default configuration and load if configuration file exists.
 */
void loadConfig (void)
{
    FILE* configFile;
    config_t configTemp;
    char path[256];

//    printf("Loading configuration...\n");

    strncpy(path, homeDir, sizeof(path));
    strncat(path, CONFIG_FILENAME, sizeof(path));
//    printf("%s: %s\n", __FUNCTION__, path);
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

    printf ("Saving configuration...\n");

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
      printf("Cannot save configuration!");
      SDL_Delay(500);
    }

    return ok;
}

/**
 * @brief loadFont Load font from file. If no file path is given it loads embedded font.
 *
 * @param aFontFilePath[in]     Font to load. It can be NULL or zero length string as well.
 * @param aFontSize[in]         Font size to use.
 * @param aWrappedScript[out]   Font of script to be filled.
 * @return TRUE: if any font successfully loaded.
 */
bool_t loadFont(const char * aFontFilePath, int aFontSize, wrappedScript_t *aWrappedScript)
{
    bool ok = TRUE;

    if (aWrappedScript->ttf_font)
    {
        printf("Releasing previous font... ");
        TTF_CloseFont(aWrappedScript->ttf_font);
        aWrappedScript->ttf_font = NULL;
        printf("Done.\n");
    }
    // Load a TrueType font
    printf("Font size: %i\n", aFontSize);
    if (aFontFilePath != NULL && strlen(aFontFilePath))
    {
        printf("Loading font '%s'... ", aFontFilePath);
        aWrappedScript->ttf_font = TTF_OpenFont(aFontFilePath, aFontSize);
        if (aWrappedScript->ttf_font != NULL)
        {
            printf("Done.\n");
        }
        else
        {
            printf("TTF_OpenFont() Failed: %s\n", TTF_GetError());
        }
    }

#if 0
    if (wrappedScript.ttf_font == NULL)
    {
        wrappedScript.ttf_font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", aFontSize);
    }
    if (wrappedScript.ttf_font == NULL)
    {
        wrappedScript.ttf_font = TTF_OpenFont("/usr/share/fonts/trutype/dejavu/DejaVuSans.ttf", aFontSize);
    }
#endif
    if (aWrappedScript->ttf_font == NULL)
    {
        printf("Loading embedded font\n");
        // Load TrueType font which is embedded into this software
        SDL_RWops* rwops = SDL_RWFromConstMem(_binary_DejaVuSans_ttf_start, (size_t)&_binary_DejaVuSans_ttf_size);
        aWrappedScript->ttf_font = TTF_OpenFontRW(rwops, 1, aFontSize);
        if (aWrappedScript->ttf_font == NULL)
        {
            printf("TTF_OpenFont() Failed: %s\n", TTF_GetError());
            ok = FALSE;
        }
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
                            (*aScriptBuffer)[fileSize] = CHR_EOS; // end of string
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

/**
 * @brief wrapScript Wrap script to the specified width.
 *
 * @param aScriptBuffer[in]     Input text to wrap.
 * @param aMaxWidthPx[in]       Maximum width of text in pixels.
 * @param aWrappedScript[out]   Wrapped text.
 * @return
 */
bool_t wrapScript(char * aScriptBuffer, uint16_t aMaxWidthPx, uint16_t aMaxHeightPx, wrappedScript_t * aWrappedScript)
{
    bool_t   ok = TRUE;
    uint32_t i;
    char   * start_ptr;         /* Start of text */
    char   * end_ptr;           /* End of text */
    char   * prev_end_ptr;      /* Previous end of text (to detect overflow of line) */
    int      text_width_px;
    int      text_height_px;
    char     text[1024] = " "; /* Empty string by default, later will be filled */
    size_t   len;
    uint32_t additional_line_count;

    printf("Wrap script to %i x %i... ", aMaxWidthPx, aMaxHeightPx);
    aWrappedScript->maxWidthPx = aMaxWidthPx;
    aWrappedScript->maxHeightPx = aMaxHeightPx;

    freeLinkedList(&aWrappedScript->wrappedScriptList);
    // Initialize iterator
    aWrappedScript->wrappedScriptList.it = &(aWrappedScript->wrappedScriptList.first);

    /* Add empty lines, so the scrolling will start with empty screen */
    TTF_SizeUTF8(aWrappedScript->ttf_font, text, &text_width_px, &text_height_px);
    aWrappedScript->linePerScreen = aMaxHeightPx / text_height_px;
    additional_line_count = aWrappedScript->linePerScreen + 4;
    for (i = 0u; i < additional_line_count && ok; i++)
    {
        if (i == additional_line_count - 6)
        {
            text[0] = '3';
        }
        if (i == additional_line_count - 5)
        {
            text[0] = ' ';
        }
        if (i == additional_line_count - 4)
        {
            text[0] = '2';
        }
        if (i == additional_line_count - 3)
        {
            text[0] = ' ';
        }
        if (i == additional_line_count - 2)
        {
            text[0] = '1';
        }
        if (i == additional_line_count - 1)
        {
            text[0] = ' ';
        }
        ok = addScriptElement(text, &(aWrappedScript->wrappedScriptList));
    }

    start_ptr = aScriptBuffer;
    end_ptr = start_ptr;
    prev_end_ptr = start_ptr;

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
                strncpy(text, start_ptr, len);
                text[len] = CHR_EOS; // end of string
//                printf("text: [%s]\n", text);
                TTF_SizeUTF8(aWrappedScript->ttf_font, text, &text_width_px, &text_height_px);
//                printf("width_px: %i height_px: %i\n", text_width_px, text_height_px);

                // Check if next word is longer than necessary
                if (text_width_px >= aMaxWidthPx)
                {
                    // It's longer, wrap text at previous word
                    len = (uintptr_t)prev_end_ptr - (uintptr_t)start_ptr;
                    strncpy(text, start_ptr, len);
                    text[len] = CHR_EOS; // end of string
                    ok = addScriptElement(text, &(aWrappedScript->wrappedScriptList));
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
        // Add last chunk of text
        len = (uintptr_t)&aScriptBuffer[i] - (uintptr_t)start_ptr;
        strncpy(text, start_ptr, len);
        text[len] = CHR_EOS; // end of string
        addScriptElement(text, &(aWrappedScript->wrappedScriptList));
        aWrappedScript->wrappedScriptList.actual = aWrappedScript->wrappedScriptList.first;
        aWrappedScript->wrappedScriptHeightPx = text_height_px;
    }
    else
    {
        /* Error occurred: free linked list */
        // FIXME display some text?
        freeLinkedList(&aWrappedScript->wrappedScriptList);
    }
    printf("Done.\n");

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

void initScreen(void)
{
    Uint32 flags = SDL_SWSURFACE;

    if (screen)
    {
        SDL_FreeSurface(screen);
    }

    if (config.full_screen)
    {
        flags |= SDL_FULLSCREEN;
    }
    screen = SDL_SetVideoMode(config.video_size_x_px, config.video_size_y_px, config.video_depth_bit, flags);

    if (background)
    {
        SDL_FreeSurface(background);
    }
    if (alphaSurface)
    {
        SDL_FreeSurface(alphaSurface);
    }
    // Create background image
    background = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                      config.video_size_x_px, config.video_size_y_px, config.video_depth_bit,
                                      config.background_color.r, config.background_color.g, config.background_color.b, 0);

    alphaSurface = SDL_CreateRGBSurface(SDL_SRCALPHA,
                                        config.video_size_x_px, config.video_size_y_px, config.video_depth_bit,
                                        0, 0, 0, 0xFF);

#if 1
    uint16_t y;
    for (y = 0; y < 100; y++)
    {
        lineRGBA(alphaSurface, 0, y, config.video_size_x_px - 1, y, 0xFF, 0xFF, 0xFF, y);
    }
#endif

    //Apply image to screen
    SDL_BlitSurface(background, NULL, screen, NULL);
}


void initTimer(void)
{
    Uint32 delay = (config.auto_scroll_speed ^ UINT8_MAX);
    if (autoScrollTimer)
    {
        SDL_RemoveTimer(autoScrollTimer);
    }
    autoScrollTimer = SDL_AddTimer(delay, timerCallbackFunc, NULL);
}


void printHelp(const char * cmd)
{
    printf("Usage:\n"
           "%s [-s <script.txt>] [-f <font.ttf>] [-i] [-S <font size>] [-tw <width%%>] [-th <height%%>]\n"
           "\n"
           "Switches:\n"
           "-s or --script: load script to display\n"
           "-f or --font: load TrueType font to be used to display script\n"
           "-i or --internal-font: use internal font\n"
           "-S or --font-size: specify font size\n"
           "-tw or --text-width-percent: display text width in percent\n"
           "-th or --text-height-percent: display text height in percent\n"
           "-vx or --video-size-x: screen size in direction X in pixels. Default: 640.\n"
           "-vy or --video-size-y: screen size in direction Y in pixels. Default: 480.\n"
           "-vd or --video-depth-bit: pixel depth in bits. Default: 16.\n"
           "-bgc or --background-color: background color in RGB format. Default: 0x000000 (black).\n"
           "-tc or --text-color: text color in RGB format. Default: 0xFFFFFF (white).\n"
           "-c or --align-center: align text to center. Default.\n"
           "-l or --align-left: align text to left.\n"
           "-a or --auto-scroll-speed: specify speed of auto scrolling. Default: 240.\n"
           "\n"
           , cmd);
}

/**
 * @brief getColor Convert decimal/hexadecimal color to SDL color.
 *
 * @param str   RGB color in hexadecimal or decimal format. Example for red: 0xFF0000.
 * @return SDL color.
 */
SDL_Color getSDLColor(const char *str)
{
    SDL_Color sdl_color;
    uint32_t color;

    if (str[0] == '0' && str[1] == 'x')
    {
        color = strtol(&str[2], NULL, 16);
    }
    else
    {
        color = atoi(str);
    }

    sdl_color.r = (color >> 16) & 0xFF;
    sdl_color.g = (color >> 8) & 0xFF;
    sdl_color.b = color & 0xFF;

    return sdl_color;
}

/**
 * @brief initArgs Parse command line arguments and change configuration according to that.
 *
 * @param argc  Count of arguments.
 * @param argv  Array of arguments.
 * @return TRUE: if no error was detected.
 */
bool_t initArgs (int argc, char* argv[])
{
    uint8_t i;
    char  * arg;
    bool_t  ok = TRUE;

    for (i = 1; i < argc; i++)
    {
        arg = argv[i];
        printf("argv: %s\n", arg);
        if (!strcmp(arg, "-s") || !strcmp(arg, "--script"))
        {
            /* Script file path */
            i++;
            arg = argv[i];
            strncpy(config.script_file_path, arg, sizeof(config.script_file_path));
        }
        else if (!strcmp(arg, "-f") || !strcmp(arg, "--font"))
        {
            /* Font file path */
            i++;
            arg = argv[i];
            strncpy(config.ttf_file_path, arg, sizeof(config.ttf_file_path));
        }
        else if (!strcmp(arg, "-i") || !strcmp(arg, "--internal-font"))
        {
            /* Use internal font */
            strncpy(config.ttf_file_path, "", sizeof(config.ttf_file_path));
        }
        else if (!strcmp(arg, "-S") || !strcmp(arg, "--font-size"))
        {
            /* Font size */
            i++;
            arg = argv[i];
            config.ttf_size = atoi(arg);
        }
        else if (!strcmp(arg, "-tw") || !strcmp(arg, "--text-width-percent"))
        {
            /* Text width in percent */
            i++;
            arg = argv[i];
            config.text_width_percent = atoi(arg);
        }
        else if (!strcmp(arg, "-th") || !strcmp(arg, "--text-height-percent"))
        {
            /* Text height in percent */
            i++;
            arg = argv[i];
            config.text_height_percent = atoi(arg);
        }
        else if (!strcmp(arg, "-vx") || !strcmp(arg, "--video-size-x"))
        {
            /* Video size in X direction */
            i++;
            arg = argv[i];
            config.video_size_x_px = atoi(arg);
        }
        else if (!strcmp(arg, "-vy") || !strcmp(arg, "--video-size-y"))
        {
            /* Video size in X direction */
            i++;
            arg = argv[i];
            config.video_size_y_px = atoi(arg);
        }
        else if (!strcmp(arg, "-vd") || !strcmp(arg, "--video-depth-bit"))
        {
            /* Video depth in bits */
            i++;
            arg = argv[i];
            config.video_depth_bit = atoi(arg);
        }
        else if (!strcmp(arg, "-bgc") || !strcmp(arg, "--background-color"))
        {
            /* Background color */
            i++;
            arg = argv[i];
            config.background_color = getSDLColor(arg);
        }
        else if (!strcmp(arg, "-tc") || !strcmp(arg, "--text-color"))
        {
            /* Text color */
            i++;
            arg = argv[i];
            config.text_color = getSDLColor(arg);
        }
        else if (!strcmp(arg, "-c") || !strcmp(arg, "--align-center"))
        {
            /* Align text to center */
            config.align_center = TRUE;
        }
        else if (!strcmp(arg, "-l") || !strcmp(arg, "--align-left"))
        {
            /* Align text to left */
            config.align_center = FALSE;
        }
        else if (!strcmp(arg, "-a") || !strcmp(arg, "--auto-scroll-speed"))
        {
            /* Speed of automatic text scroll */
            i++;
            arg = argv[i];
            config.auto_scroll_speed = atoi(arg);
        }
        else if (!strcmp(arg, "-h") || !strcmp(arg, "--help"))
        {
            printHelp(argv[0]);
            exit(1);
        }
        else
        {
            printf("ERROR: unknown parameter!\n"
                   "\n"
                   "See help: -h\n");
        }
    }

    return ok;
}

/**
 * @brief init
 * Initialize teleprompter.
 *
 * @return TRUE: if successfully initialized. FALSE: error occurred.
 */
bool_t init (int argc, char* argv[])
{
    uint8_t i;
    char    path[256];
    const SDL_VideoInfo * videoInfo;

    setlocale(LC_ALL, ""); // FIXME needed?
    srand(time(NULL));

    printf("This is Delta Teleprompter.\n"
           "\n"
           "Copyright (C) Peter Ivanov <ivanovp@gmail.com>, 2021\n"
           "Homepage: http://dev.ivanov.eu\n"
           "Licence: GPLv3\n"
           "\n"
           "This program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.\n"
           "This is free software, and you are welcome to redistribute it under certain\n"
           "conditions; see LICENSE for details.\n"
           "\n"
           );

    homeDir = getenv ("HOME");
    if (homeDir == NULL)
    {
      homeDir = getpwuid (getuid ())->pw_dir;
    }
    strncpy(path, homeDir, sizeof(path));
    strncat(path, CONFIG_DIR, sizeof(path));
//    printf("%s mkdir: %s\n", __FUNCTION__, path);
    mkdir(path, 0755);

    loadConfig ();

    initArgs(argc, argv);

    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
//    SDL_Init(SDL_INIT_EVERYTHING);

    videoInfo = SDL_GetVideoInfo();

    printf("Hardware surface available: %i\n",      videoInfo->hw_available);	/**< Flag: Can you create hardware surfaces? */
    printf("Window manager available: %i\n",        videoInfo->wm_available);	/**< Flag: Can you talk to a window manager? */
    printf("Accelerated blits HW->HW: %i\n",        videoInfo->blit_hw     );	/**< Flag: Accelerated blits HW --> HW */
    printf("Accelerated blits with Colorkey: %i\n", videoInfo->blit_hw_CC  );	/**< Flag: Accelerated blits with Colorkey */
    printf("Accelerated blits with Alpha: %i\n",    videoInfo->blit_hw_A   );	/**< Flag: Accelerated blits with Alpha */
    printf("Accelerated blits SW->HW: %i\n",        videoInfo->blit_sw     );	/**< Flag: Accelerated blits SW --> HW */
    printf("Accelerated blits with Colorkey: %i\n", videoInfo->blit_sw_CC  );	/**< Flag: Accelerated blits with Colorkey */
    printf("Accelerated blits with Alpha: %i\n",    videoInfo->blit_sw_A   );	/**< Flag: Accelerated blits with Alpha */
    printf("Accelerated color fill: %i\n",          videoInfo->blit_fill   );	/**< Flag: Accelerated color fill */
    printf("Actual screen size: %i x %i\n",         videoInfo->current_w, videoInfo->current_h);
    printf("Video memory: %i KiB\n",                videoInfo->video_mem);

    printf("Configuration version: %i\n", config.version);
    printf("Script file path:      %s\n", config.script_file_path);
    printf("Font file path:        %s\n", config.ttf_file_path);
    printf("Font size:             %i\n", config.ttf_size);
    printf("Text width:            %i%%\n", config.text_width_percent);
    printf("Text height:           %i%%\n", config.text_height_percent);
    printf("Requested screen size: %i x %i x %i\n", config.video_size_x_px, config.video_size_y_px, config.video_depth_bit);
    printf("Background color:      %02X %02X %02X\n", config.background_color.r, config.background_color.g, config.background_color.b);
    printf("Text color:            %02X %02X %02X\n", config.text_color.r, config.text_color.g, config.text_color.b);
    printf("Align center:          %i\n", config.align_center);
    printf("Auto scroll speed:     %i\n", config.auto_scroll_speed);
    printf("Scroll line count:     %i\n", config.scroll_line_count);
    printf("Full screen:           %i\n", config.full_screen);

    initScreen();
    initTimer();

    // Initialize SDL_ttf library
    if (TTF_Init() != 0)
    {
        printf("TTF_Init() Failed: %s\n", TTF_GetError());
        SDL_Quit();
        exit(1);
    }

    for (i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
    {
        keys[i].repeatTick = NORMAL_REPEAT_TICK;
    }
    keys[KEY_UP].repeatTick = FAST_REPEAT_TICK;
    keys[KEY_DOWN].repeatTick = FAST_REPEAT_TICK;
    keys[KEY_LEFT].repeatTick = FAST_REPEAT_TICK;
    keys[KEY_RIGHT].repeatTick = FAST_REPEAT_TICK;

    return TRUE;
}

/**
 * @brief scrollScriptUpPx Scroll text up by one pixel.
 *
 * @param aWrappedScript[in] Script to be scrolled.
 */
void scrollScriptUpPx(wrappedScript_t * aWrappedScript)
{
    aWrappedScript->heightOffsetPx++;
    if (aWrappedScript->heightOffsetPx == aWrappedScript->wrappedScriptHeightPx
            && aWrappedScript->wrappedScriptList.actual)
    {
        if (aWrappedScript->wrappedScriptList.actual->next)
        {
            /* Advance to next line */
            aWrappedScript->wrappedScriptList.actual = aWrappedScript->wrappedScriptList.actual->next;
            aWrappedScript->heightOffsetPx = 0;
            aWrappedScript->isEnd = FALSE;
        }
        else
        {
            aWrappedScript->isEnd = TRUE;
        }
    }
}

/**
 * @brief scrollScriptUp Scroll text up by specified count of lines.
 *
 * @param aWrappedScript[in]    Script to be scrolled.
 * @param lineCount[in]         Line count to scroll.
 */
void scrollScriptUp(wrappedScript_t * aWrappedScript, int lineCount)
{
    while (lineCount > 0)
    {
        if (aWrappedScript->wrappedScriptList.actual)
        {
            if (aWrappedScript->wrappedScriptList.actual->next)
            {
                aWrappedScript->wrappedScriptList.actual = aWrappedScript->wrappedScriptList.actual->next;
                aWrappedScript->isEnd = FALSE;
            }
            else
            {
                aWrappedScript->isEnd = TRUE;
            }
        }
        lineCount--;
    }
}

/**
 * @brief scrollScriptDown Scroll text down by specified count of lines.
 *
 * @param aWrappedScript[in]    Script to be scrolled.
 * @param lineCount[in]         Line count to scroll.
 */
void scrollScriptDown(wrappedScript_t * aWrappedScript, int lineCount)
{
    while (lineCount > 0)
    {
        if (aWrappedScript->wrappedScriptList.actual
                && aWrappedScript->wrappedScriptList.actual->prev)
        {
            aWrappedScript->wrappedScriptList.actual = aWrappedScript->wrappedScriptList.actual->prev;
        }
        lineCount--;
    }
}

/**
 * @brief handleTeleprompterKeys
 * Handle button presses and move text according to that.
 */
void handleTeleprompterKeys (void)
{
    bool ok;
    bool loadFontWrap = FALSE;

    if (IS_PRESSED_CHANGED(KEY_RIGHT))
    {
        /* Right */
        if (config.auto_scroll_speed < 255)
        {
            config.auto_scroll_speed++;
            initTimer();
        }
        printf("Auto scroll speed: %i\n", config.auto_scroll_speed);
        drawInfoScreen("Auto scroll speed: %i", config.auto_scroll_speed);
    }
    else if (IS_PRESSED_CHANGED(KEY_LEFT))
    {
        /* Left */
        if (config.auto_scroll_speed > 0)
        {
            config.auto_scroll_speed--;
            initTimer();
        }
        printf("Auto scroll speed: %i\n", config.auto_scroll_speed);
        drawInfoScreen("Auto scroll speed: %i", config.auto_scroll_speed);
    }
    if (IS_PRESSED_CHANGED(KEY_PLUS))
    {
        if (config.ttf_size < MAX_FONT_SIZE)
        {
            config.ttf_size += FONT_SIZE_STEP;
            loadFontWrap = TRUE;
        }
    }
    else if (IS_PRESSED_CHANGED(KEY_MINUS))
    {
        if (config.ttf_size > MIN_FONT_SIZE)
        {
            config.ttf_size -= FONT_SIZE_STEP;
            loadFontWrap = TRUE;
        }
    }
    if (IS_PRESSED_CHANGED(KEY_UP))
    {
        /* Scroll script up */
        scrollScriptUp(&wrappedScript, config.scroll_line_count);
    }
    else if (IS_PRESSED_CHANGED(KEY_DOWN))
    {
        /* Scroll script down */
        scrollScriptDown(&wrappedScript, config.scroll_line_count);
    }
    if (IS_PRESSED_CHANGED(KEY_F5))
    {
        if (config.text_width_percent > MIN_TEXT_WIDTH_PERCENT)
        {
            config.text_width_percent -= TEXT_WIDTH_PERCENT_STEP;
            loadFontWrap = TRUE;
        }
        printf("Text width: %i%%\n", config.text_width_percent);
        drawInfoScreen("Text width: %i%%", config.text_width_percent);
    }
    if (IS_PRESSED_CHANGED(KEY_F6))
    {
        if (config.text_width_percent < MAX_TEXT_WIDTH_PERCENT)
        {
            config.text_width_percent += TEXT_WIDTH_PERCENT_STEP;
            loadFontWrap = TRUE;
        }
        printf("Text width: %i%%\n", config.text_width_percent);
        drawInfoScreen("Text width: %i%%", config.text_width_percent);
    }
    if (IS_PRESSED_CHANGED(KEY_F7))
    {
        if (config.text_height_percent > MIN_TEXT_HEIGHT_PERCENT)
        {
            config.text_height_percent -= TEXT_HEIGHT_PERCENT_STEP;
            wrappedScript.maxHeightPx = (float)config.video_size_y_px * config.text_height_percent / 100.0f;
        }
        printf("Text height: %i%%\n", config.text_height_percent);
        drawInfoScreen("Text height: %i%%", config.text_height_percent);
    }
    if (IS_PRESSED_CHANGED(KEY_F8))
    {
        if (config.text_height_percent < MAX_TEXT_HEIGHT_PERCENT)
        {
            config.text_height_percent += TEXT_HEIGHT_PERCENT_STEP;
            wrappedScript.maxHeightPx = (float)config.video_size_y_px * config.text_height_percent / 100.0f;
        }
        printf("Text height: %i%%\n", config.text_height_percent);
        drawInfoScreen("Text height: %i%%", config.text_height_percent);
    }
    if (IS_PRESSED_CHANGED(KEY_F11))
    {
        config.full_screen = !config.full_screen;
        initScreen();
    }

    if (loadFontWrap)
    {
        ok = loadFont(config.ttf_file_path, config.ttf_size, &wrappedScript);
        if (ok)
        {
            wrapScript(scriptBuffer,
                       (float)config.video_size_x_px * config.text_width_percent / 100.0f,
                       (float)config.video_size_y_px * config.text_height_percent / 100.0f,
                       &wrappedScript);
        }
    }
}

/**
 * @brief handle_main_state_machine
 * Check inputs and change state machine if it is necessary.
 */
void handleMainStateMachine (void)
{
    bool    ok;

    switch (main_state_machine)
    {
        case STATE_intro:
            introTimer--;
            if (IS_PRESSED_CHANGED(KEY_ENTER) || IS_PRESSED_CHANGED(KEY_SPACE)
                    || !introTimer)
            {
                main_state_machine = STATE_load_script;
            }
            drawHelpScreen();
            break;
        case STATE_help:
            if (IS_PRESSED_CHANGED(KEY_ENTER) || IS_PRESSED_CHANGED(KEY_SPACE)
                    || IS_PRESSED_CHANGED(KEY_F1))
            {
                main_state_machine = main_state_machine_next;
            }
            drawHelpScreen();
            break;
        case STATE_load_script:
            ok = loadFont(config.ttf_file_path, config.ttf_size, &wrappedScript);
            if (ok)
            {
                ok = loadScript(config.script_file_path, &scriptBuffer);
            }
            if (ok)
            {
                ok = wrapScript(scriptBuffer,
                                (float)config.video_size_x_px * config.text_width_percent / 100.0f,
                                (float)config.video_size_y_px * config.text_height_percent / 100.0f,
                                &wrappedScript);
            }
            if (ok)
            {
                /* Script successfully loaded, immediately show it */
                main_state_machine = STATE_running;
            }
            else
            {
                /* Error occured, leave error message on the screen for a while */
                main_state_machine_next = STATE_end;
                main_state_machine = STATE_load_script_wait;
            }
            wrappedScript.isEnd = FALSE;
            break;
        case STATE_load_script_wait:
            loadScriptTimer--;
            if (!loadScriptTimer)
            {
                main_state_machine = main_state_machine_next;
            }
            break;
        case STATE_running:
            handleTeleprompterKeys ();
            if (IS_PRESSED_CHANGED(KEY_ENTER) || IS_PRESSED_CHANGED(KEY_SPACE))
            {
                main_state_machine = STATE_paused;
            }
            if (IS_PRESSED_CHANGED(KEY_F1))
            {
                main_state_machine = STATE_help;
                main_state_machine_next = STATE_running;
            }
            drawScreen ();
            if (wrappedScript.isEnd)
            {
                main_state_machine = STATE_end;
            }
            break;
        case STATE_paused:
            handleTeleprompterKeys();
            if (IS_PRESSED_CHANGED(KEY_ENTER) || IS_PRESSED_CHANGED(KEY_SPACE))
            {
                main_state_machine = STATE_running;
            }
            if (IS_PRESSED_CHANGED(KEY_F1))
            {
                main_state_machine = STATE_help;
                main_state_machine_next = STATE_paused;
            }
            drawScreen ();
            if (wrappedScript.isEnd)
            {
                main_state_machine = STATE_end;
            }
            break;
        case STATE_end:
            if (IS_PRESSED_CHANGED(KEY_ENTER) || IS_PRESSED_CHANGED(KEY_SPACE))
            {
                /* Restart teleprompter */
                introTimer = DEFAULT_INTRO_TIMER;
                loadScriptTimer = DEFAULT_LOAD_SCRIPT_TIMER;
                main_state_machine = STATE_load_script;
            }
            if (IS_PRESSED_CHANGED(KEY_F1))
            {
                main_state_machine = STATE_help;
                main_state_machine_next = STATE_end;
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
    if (key_index < KEY_COUNT)
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
        printf("%s internal error\n", __FUNCTION__);
    }
}

bool_t eventHandler()
{
    int i;
    SDL_Event event;
    bool_t    eventOccurred = FALSE;

    for (i = 0; i < KEY_COUNT; i++)
    {
        keys[i].changed = FALSE;
        if (keys[i].pressed && keys[i].pressTick < SDL_GetTicks())
        {
            /* Simulate key has just pressed */
            keys[i].changed = TRUE;
            keys[i].pressTick = SDL_GetTicks() + keys[i].repeatTick;
            eventOccurred = TRUE;
        }
    }

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_USEREVENT)
        {
            if (TELEPROMPTER_IS_RUNNING())
            {
                scrollScriptUpPx(&wrappedScript);
            }
        }
        else if (event.type == SDL_KEYDOWN)
        {
            eventOccurred = TRUE;
            if (textInputIsStarted && text)
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
                case SDLK_PLUS:
                case SDLK_KP_PLUS:
                    key_pressed(KEY_PLUS, TRUE);
                    break;
                case SDLK_MINUS:
                case SDLK_KP_MINUS:
                    key_pressed(KEY_MINUS, TRUE);
                    break;
                case SDLK_F1:
                    key_pressed(KEY_F1, TRUE);
                    break;
                case SDLK_F2:
                    key_pressed(KEY_F2, TRUE);
                    break;
                case SDLK_F3:
                    key_pressed(KEY_F3, TRUE);
                    break;
                case SDLK_F4:
                    key_pressed(KEY_F4, TRUE);
                    break;
                case SDLK_F5:
                    key_pressed(KEY_F5, TRUE);
                    break;
                case SDLK_F6:
                    key_pressed(KEY_F6, TRUE);
                    break;
                case SDLK_F7:
                    key_pressed(KEY_F7, TRUE);
                    break;
                case SDLK_F8:
                    key_pressed(KEY_F8, TRUE);
                    break;
                case SDLK_F9:
                    key_pressed(KEY_F9, TRUE);
                    break;
                case SDLK_F10:
                    key_pressed(KEY_F10, TRUE);
                    break;
                case SDLK_F11:
                    key_pressed(KEY_F11, TRUE);
                    break;
                case SDLK_F12:
                    key_pressed(KEY_F12, TRUE);
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
            /* Releasing key should not be an event as it disturbs drawInfoScreen() */
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
                case SDLK_PLUS:
                case SDLK_KP_PLUS:
                    key_pressed(KEY_PLUS, FALSE);
                    break;
                case SDLK_MINUS:
                case SDLK_KP_MINUS:
                    key_pressed(KEY_MINUS, FALSE);
                    break;
                case SDLK_F1:
                    key_pressed(KEY_F1, FALSE);
                    break;
                case SDLK_F2:
                    key_pressed(KEY_F2, FALSE);
                    break;
                case SDLK_F3:
                    key_pressed(KEY_F3, FALSE);
                    break;
                case SDLK_F4:
                    key_pressed(KEY_F4, FALSE);
                    break;
                case SDLK_F5:
                    key_pressed(KEY_F5, FALSE);
                    break;
                case SDLK_F6:
                    key_pressed(KEY_F6, FALSE);
                    break;
                case SDLK_F7:
                    key_pressed(KEY_F7, FALSE);
                    break;
                case SDLK_F8:
                    key_pressed(KEY_F8, FALSE);
                    break;
                case SDLK_F9:
                    key_pressed(KEY_F9, FALSE);
                    break;
                case SDLK_F10:
                    key_pressed(KEY_F10, FALSE);
                    break;
                case SDLK_F11:
                    key_pressed(KEY_F11, FALSE);
                    break;
                case SDLK_F12:
                    key_pressed(KEY_F12, FALSE);
                    break;
                case SDLK_ESCAPE:
                    break;
                default:
                    break;
            }
        }
        else if (event.type == SDL_QUIT) /* If the user has Xed out the window */
        {
            eventOccurred = TRUE;
            /* Quit the program */
            teleprompterRunning = FALSE;
        }
    }

    return eventOccurred;
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
        eventHandler();
        handleMainStateMachine ();
        SDL_Delay(1);
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
        freeLinkedList(&wrappedScript.wrappedScriptList);
        printf("Done\n");
    }

    TTF_CloseFont(wrappedScript.ttf_font);

    //Free the surfaces
    SDL_FreeSurface(background);
    SDL_FreeSurface(alphaSurface);
    SDL_FreeSurface(screen);

    // Quit TTF
    TTF_Quit();
    // Quit SDL
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    if (init (argc, argv))
    {
        run ();
        done ();
    }

    return 0;
}

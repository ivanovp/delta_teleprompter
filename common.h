/**
 * @file        common.h
 * @brief       Common definitions
 * @author      Copyright (C) Peter Ivanov, 2011, 2012, 2013, 2014, 2021
 *
 * Created      2011-01-19 11:48:53
 * Last modify: 2021-02-16 17:51:49 ivanovp {Time-stamp}
 * Licence:     GPL
 */

#ifndef INCLUDE_COMMON_H
#define INCLUDE_COMMON_H

#include <stdint.h>
#include <SDL/SDL.h>

#define VERSION_MAJOR    1
#define VERSION_MINOR    1
#define VERSION_REVISION 1

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

#ifndef FALSE
#define FALSE   (0)
#endif
#ifndef TRUE
#define TRUE    (1)
#endif

#define CHR_EOS                     '\0'    /* End of string */
#define CHR_SPACE                   ' '
#define CHR_CR                      '\r'    /* Carriage return */
#define CHR_LF                      '\n'    /* Line feed */
#define CHR_TAB                     '\t'    /* Tabulator */
#define IS_WHITESPACE(c)    ((c) == CHR_SPACE || (c) == CHR_CR || (c) == CHR_LF || (c) == CHR_TAB)

#define BIT0    0x01u
#define BIT1    0x02u
#define BIT2    0x04u
#define BIT3    0x08u
#define BIT4    0x10u
#define BIT5    0x20u
#define BIT6    0x40u
#define BIT7    0x80u

#define _BV(x)  (1u << (x))

#define TELEPROMPTER_IS_PAUSED()    (main_state_machine == STATE_paused)
#define TELEPROMPTER_IS_RUNNING()   (main_state_machine == STATE_running)
#define TELEPROMPTER_IS_FINISHED()  (main_state_machine == STATE_end)

#define FSYS_FILENAME_MAX   255 // FIXME inherited from dingoo
#define OS_TICKS_PER_SEC    1000

typedef char bool_t;

typedef enum
{
    STATE_undefined,            /**< This shall not be used! */
    STATE_intro,                /**< Show info text. */
    STATE_help,                 /**< Show help. */
    STATE_load_script,          /**< Load script if it has not already loaded. */
    STATE_load_script_wait,
    STATE_running,              /**< Teleprompter shows text */
    STATE_paused,               /**< Teleprompter is paused. It can be started again. */
    STATE_end,                  /**< End of text, it can be started again. */
    STATE_size        /**< Not a real state. Only to count number of states. THIS SHOULD BE THE LAST ONE! */
} main_state_machine_t;

typedef struct
{
    uint8_t     version;        /* To prevent loading invalid configuration */
    char        script_file_path[FSYS_FILENAME_MAX];
    char        ttf_file_path[FSYS_FILENAME_MAX];
    uint16_t    ttf_size;
    uint16_t    text_width_percent;
    uint16_t    text_height_percent;
    uint16_t    video_size_x_px;
    uint16_t    video_size_y_px;
    uint8_t     video_depth_bit;
    SDL_Color   background_color;
    SDL_Color   text_color;
    bool_t      align_center;
    uint8_t     auto_scroll_speed;
    uint8_t     scroll_line_count;
    bool_t      full_screen;
    bool_t      text_fading;
    bool_t      verbose;
} config_t;

/* Teleprompter related */
extern uint32_t     teleprompterTimer;
extern bool_t       teleprompterRunning;
extern main_state_machine_t main_state_machine; /* Teleprompter state machine. @see handleMainStateMachine */

extern config_t     config; /* Actual configuration. */

bool_t saveConfig (void);
bool_t eventHandler();

#endif /* INCLUDE_COMMON_H */


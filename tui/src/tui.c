/*****************************************************************************
 *
 *    tui/src/tui.c
 *    This file is part of TMCScan distribution released under GPL 3.0
 *    Copyright (C) 2022 _Magenta_
 *    https://github.com/0Magenta0/TMCScan
 *
 *****************************************************************************/

#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>

#include "tui/tui_params.h"
#include "tui/tui_utils.h"
#include "tui/tui_acts.h"
#include "tui/tui_scan.h"
#include "tui/tui_quit.h"
#include "tui/tui.h"

#include "scan/scan.h"

#define MIN_LINES 15
#define MIN_COLS 50
#define PARAMS_COUNT 5
#define ACTS_COUNT 2

void print_main_box (const int, const int);
void print_servers (const unsigned int, const unsigned int, const int);
void print_server (const unsigned int, const unsigned int, const unsigned int, const char);
int check_mouse_pos_serv_list (const int, const int, const MEVENT *);
void print_current_item_num (const int, const int);

int ltc_menu_pos_x; /* Left-Top corner of menu position by horizontal */

unsigned int current_serv_item,
             items_shift;

const struct param params_list [PARAMS_COUNT] = { { "ip", 2, 15, 'i' },
                                                  { "port", 4, 5, 'p' },
                                                  { "online", 6, 10, 'o' },
                                                  { "version", 7, 27, 'v' },
                                                  { "motd", 4, 20, 'm' } };

struct action acts_list [ACTS_COUNT] = { { "scan", 4, act_scan, 's', 0 },
                                         { "quit", 4, act_quit, 'q', 0 } };

int ini_curses (void) {
    setlocale (LC_ALL, "");
    initscr ();
    if (keypad (stdscr, 1) == ERR)
        return 0;

    mousemask (ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, 0);
    nodelay (stdscr, 1);
    curs_set (0);
    noecho ();
    raw ();

    if (has_colors ())
        start_color ();
    else
        return 0;

    if (can_change_color ()) {
        assume_default_colors (231, 16);
        init_pair (1, 196, 16);
        init_pair (2, 112, 16);
        init_pair (3, 80, 16);
    } else {
        init_pair (1, COLOR_RED, COLOR_BLACK);
        init_pair (2, COLOR_GREEN, COLOR_BLACK);
        init_pair (3, COLOR_CYAN, COLOR_BLACK);
    }

    return 1;
}

void show_menu (void) {
    MEVENT mouse_event;
    register int i, ch, y, x;

    getmaxyx (stdscr, y, x);
    if (y < MIN_LINES || x < MIN_COLS) {
        print_min_size (MIN_LINES, MIN_COLS, y, x);
    } else {
        print_main_box (y, x);
        print_current_item_num (y, x);
    }

    while ((ch = getch ()) && is_open) {
        if (scan_status == scan_status_scanning) {
            if (scan_status == scan_status_scanning && !is_scanning) {
                items_shift = 0;
                current_serv_item = 0;
                scan_status = scan_status_end;
                copy_serv_list_to_array ();
                free (scan_threads);
            } if (ch == KEY_RESIZE) {
                getmaxyx (stdscr, y, x);
            } if (y < MIN_LINES || x < MIN_COLS) {
                    print_min_size (MIN_LINES, MIN_COLS, y, x);
                    refresh ();
                    goto _key_loop_end;
            }

            print_main_box (y, x);
            print_servers (y - 2, x - 4, OK);
            acts_list [0].func (0, OK, y, x, acts_list [0].name);
            goto _key_loop_end;
        }

        if (ch == ERR)
            goto _key_loop_end;

        if (ch == KEY_MOUSE && getmouse (&mouse_event) == ERR)
            goto _key_loop_end;

        if (ch == KEY_RESIZE) {
            getmaxyx (stdscr, y, x);
            if (y < MIN_LINES || x < MIN_COLS) {
                print_min_size (MIN_LINES, MIN_COLS, y, x);
                continue;
            }

            print_main_box (y, x);
            print_servers (y - 2, x - 4, OK);
            for (i = 0; i < ACTS_COUNT; ++i)
                if (acts_list [i].enabled)
                    acts_list [i].enabled = acts_list [i].func (0, ch, y, x, acts_list [i].name);

        } else {
            if (y < MIN_LINES || x < MIN_COLS) {
                print_min_size (MIN_LINES, MIN_COLS, y, x);
                continue;
            } for (i = 0; i < ACTS_COUNT; ++i) {
                if (acts_list [i].enabled) {
                    if (!(acts_list [i].enabled = acts_list [i].func (ch == KEY_MOUSE ? &mouse_event : 0, ch, y, x, acts_list [i].name))) {
                        print_main_box (y, x);
                        print_servers (y - 2, x - 4, OK);
                    } goto _key_loop_end;
                }
            } for (i = 0; i < ACTS_COUNT; ++i) {
                if (acts_list [i].bind == ch) {
                    acts_list [i].enabled = 1;
                    acts_list [i].func (0, OK, y, x, acts_list [i].name);
                    goto _key_loop_end;
                }
            } for (i = 0; i < PARAMS_COUNT; ++i) {
                if (params_list [i].bind == ch) {
                    sort_servers (i);
                    print_main_box (y, x);
                    print_servers (y - 2, x - 4, OK);
                    goto _key_loop_end;
                }
            } if (check_move_key (ch)) {
                print_main_box (y, x);
                print_servers (y - 2, x - 4, ch);
            } else if (check_enter_key (ch)) {
                /* TODO: Handle This Type Of Keys */
            } else if (check_mouse_click (&mouse_event)) {
                if ((i = check_mouse_pos_params (&mouse_event, params_list, PARAMS_COUNT)) >= 0) {
                    sort_servers (i);
                    print_main_box (y, x);
                    print_servers (y - 2, x - 4, OK);
                } else if ((i = check_mouse_pos_acts (y - 1, &mouse_event, acts_list, ACTS_COUNT)) >= 0) {
                    acts_list [i].enabled = 1;
                    acts_list [i].func (0, OK, y, x, acts_list [i].name);
                    goto _key_loop_end;
                } else if ((i = check_mouse_pos_serv_list (y - 1, x - 2, &mouse_event)) >= 0) {
                    current_serv_item = i;
                    print_main_box (y, x);
                    print_servers (y - 2, x - 4, OK);
                }
            } else if ((i = check_mouse_double_click (&mouse_event)) >= 0) {
                if ((i = check_mouse_pos_params (&mouse_event, params_list, PARAMS_COUNT)) >= 0) {
                    sort_servers (i);
                    print_main_box (y, x);
                    print_servers (y - 2, x - 4, OK);
                } else if ((i = check_mouse_pos_acts (y - 1, &mouse_event, acts_list, ACTS_COUNT)) >= 0) {
                    acts_list [i].enabled = 1;
                    acts_list [i].func (0, OK, y, x, acts_list [i].name);
                    goto _key_loop_end;
                } else {
                    /* TODO: Server Action Dialog */
                }
            }
        }

_key_loop_end:
        if (ch == ERR) {
            usleep (60000);
        } if (scan_status == scan_status_scanning) {
            sleep (1);
        }
    }
}

void print_main_box (const int y, const int x) {
    int i, x_pos;

    clear ();
    box (stdscr, 0, 0);
    for (i = 0, x_pos = 2; i < PARAMS_COUNT; x_pos += 1 + params_list [i].len, ++i)
        print_param (x, x_pos, &params_list [i]);

    for (i = 0, x_pos = 1; i < ACTS_COUNT; x_pos += 2 + acts_list [i].len, ++i)
        print_act (y - 1, x, x_pos, &acts_list [i]);

    refresh ();
}

void print_servers (const unsigned int y, const unsigned int x, const int ch) {
    unsigned int i;

    if (ch == OK) {
        goto _print_items;
    } if (ch == 'j') {
        if (current_serv_item + 1 < serv_items_array_len)
            ++current_serv_item;

        if (current_serv_item - items_shift == y)
            ++items_shift;
    } if (ch == 'k') {
        if (current_serv_item)
            --current_serv_item;

        if (current_serv_item + 1 == items_shift)
            --items_shift;
    }

_print_items:
    print_current_item_num (y + 2, x + 4);
    if (scan_status != scan_status_scanning && serv_items_array_len) {
        for (i = 0; items_shift + i < serv_items_array_len && i < y; ++i) {
            if (items_shift + i == current_serv_item) {
                print_server (i + 1, x, items_shift + i, 1);
            } else {
                print_server (i + 1, x, items_shift + i, 0);
            }
        }
    }
}

void print_server (const unsigned int y, const unsigned int x, const unsigned int i, const char is_highlighted) {
    char *tmp_str;

    if (is_highlighted)
        attron (A_REVERSE);

    tmp_str = malloc (24);
    sprintf (tmp_str, "%d/%d", serv_items_array [i].online, serv_items_array [i].slots);
    mvprintw (y, 1, " %-*.*s %-*.*d %-*.*s %-*.*s %-*.*s",
              params_list [0].len,
              params_list [0].len,
              serv_items_array [i].ip,
              params_list [1].len,
              params_list [1].len,
              serv_items_array [i].port,
              params_list [2].len,
              params_list [2].len,
              tmp_str,
              params_list [3].len,
              params_list [3].len,
              serv_items_array [i].version,
              x - params_list [0].len - params_list [1].len - params_list [2].len - params_list [3].len - 3,
              x - params_list [0].len - params_list [1].len - params_list [2].len - params_list [3].len - 3,
              serv_items_array [i].motd);
    free (tmp_str);

    if (is_highlighted)
        attroff (A_REVERSE);
}

void fin_curses (void) {
    endwin ();
}

int check_mouse_pos_serv_list (const int y, const int x, const MEVENT *mouse_event) {
    if (items_shift + mouse_event->y - 1 < serv_items_array_len && mouse_event->y > 0 && mouse_event->y <= y && mouse_event->x > 0 && mouse_event->x <= x)
        return items_shift + mouse_event->y - 1;

    return -1;
}

void print_current_item_num (const int y, const int x) {
    int x_pos;
    char *tmp_str;

    if (serv_items_array_len) {
        tmp_str = malloc (24);
        x_pos = sprintf (tmp_str, "%d/%d", current_serv_item + 1, serv_items_array_len);
        mvaddch (y - 1, x - x_pos - 3, ACS_LRCORNER);
        attron (COLOR_PAIR (3));
        addstr (tmp_str);
        attroff (COLOR_PAIR (3));
        addch (ACS_LLCORNER);
        free (tmp_str);
    } else {
        mvaddch (y - 1, x - 6, ACS_LRCORNER);
        attron (COLOR_PAIR (1));
        addstr ("0/0");
        attroff (COLOR_PAIR (1));
        addch (ACS_LLCORNER);
    }
}


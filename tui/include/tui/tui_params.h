/*****************************************************************************
 *
 *    tui/include/tui/tui_params.h
 *    This file is part of TMCScan distribution released under GPL 3.0
 *    Copyright (C) 2022 _Magenta_
 *    https://github.com/0Magenta0/TMCScan
 *
 *****************************************************************************/

struct param {
    char *name;
    int len;
    char bind;
};

void print_param (const int, const int, const struct param *);


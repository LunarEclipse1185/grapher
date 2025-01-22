#ifndef STYLE_H_
#define STYLE_H_

#include <raylib.h>

// color constants
static const Color c_separator    = (Color) {0xD7, 0xD8, 0xD7, 0xFF};

static const Color c_bg_primary   = WHITE;
static const Color c_bg_secondary = (Color) {0xE9, 0xEA, 0xEB, 0xFF};
static const Color c_bg_tertiary = (Color) {0xD0, 0xD2, 0xD3, 0xFF};
static const Color c_bg_quaternary = (Color) {0x7E, 0x81, 0x81, 0xFF};
static const Color c_bg_highlighted = (Color) {0x65, 0x9A, 0xF9, 0xFF};

static const Color c_fg_primary   = BLACK;
static const Color c_fg_secondary = (Color) {0xD7, 0xD8, 0xD7, 0xFF};
static const Color c_fg_alarming = RED;
static const Color c_fg_placeholder = (Color) {0xB2, 0xB2, 0xB2, 0xFF};

#endif // STYLE_H_

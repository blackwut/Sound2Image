#ifndef ALLEGRO_UTIL_H
#define ALLEGRO_UTIL_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>

#include "common.h"

#define WINDOW_TITLE "Sound2Image - Alberto Ottimo"

#define FONT_NAME "font/modum.ttf"
#define FONT_SIZE_BIG 20
#define FONT_SIZE_SMALL 12


ALLEGRO_DISPLAY * display = NULL;
ALLEGRO_COLOR BACKGROUND_COLOR;

ALLEGRO_FONT * font_big = NULL;
ALLEGRO_FONT * font_small = NULL;
ALLEGRO_COLOR font_color;
ALLEGRO_EVENT_QUEUE * input_queue = NULL;

static inline void al_check(bool test, const char * description)
{
    if (test) return;
    fprintf(stderr, "ALLEGRO error in %s\n", description);
    exit(EXIT_ALLEGRO_ERROR);
}

static inline void allegro_init()
{
    al_check(al_init(), "al_init()");

    al_set_new_window_title(WINDOW_TITLE);

    // Display
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR | ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
    display = al_create_display(DISPLAY_W, DISPLAY_H);
    al_check(display, "al_create_display()");
    BACKGROUND_COLOR = al_map_rgba(32, 32, 32, 255);

    // Keyboard
    al_check(al_install_keyboard(), "al_install_keyboard()");

    // Input queue
    input_queue = al_create_event_queue();
    al_check(input_queue, "al_create_event_queue()");
    al_register_event_source(input_queue, al_get_display_event_source(display));
    al_register_event_source(input_queue, al_get_keyboard_event_source());

    // Audio
    al_init_acodec_addon();
    al_check(al_install_audio(), "al_install_audio()");
    al_check(al_reserve_samples(0), "al_reserve_samples()");

    // Font
    al_check(al_init_font_addon(), "al_init_font_addon()");
    al_check(al_init_ttf_addon(), "al_init_ttf_addon()");
    font_big = al_load_ttf_font(FONT_NAME, FONT_SIZE_BIG, 0);
    font_small = al_load_ttf_font(FONT_NAME, FONT_SIZE_SMALL, 0);
    font_color = al_map_rgba(41, 128, 185 , 255);

    // Image
    al_check(al_init_primitives_addon(), "al_init_primitives_addon()");
}

ALLEGRO_CHANNEL_CONF allegro_channel_conf_with(size_t channels)
{
    return (channels == 2 ? ALLEGRO_CHANNEL_CONF_2 : ALLEGRO_CHANNEL_CONF_1);
}

static inline void allegro_blender_mode_standard()
{
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
}

static inline void allegro_blender_mode_alpha()
{
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
}


static inline void allegro_print_text(const char * text, const float x, const float y, const int align)
{
    allegro_blender_mode_standard();
    al_draw_text(font_big, font_color, x, y, align, text);
}

static inline void allegro_print_text_small_color(const char * text, const ALLEGRO_COLOR c, const float x, const float y, const int align)
{
    allegro_blender_mode_standard();
    al_draw_text(font_small, c, x, y, align, text);
}

static inline void allegro_free()
{
    al_uninstall_keyboard();
    al_uninstall_audio();
    al_destroy_font(font_big);
    al_destroy_font(font_small);
    al_shutdown_ttf_addon();
    al_shutdown_font_addon();
    al_shutdown_primitives_addon();
    al_destroy_display(display);
    al_destroy_event_queue(input_queue);
}

#endif

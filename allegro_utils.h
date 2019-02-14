#ifndef ALLEGRO_UTIL_H
#define ALLEGRO_UTIL_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>

#include "common.h"

#define FONT_NAME "font/modum.ttf"
#define FONT_SIZE 24


ALLEGRO_DISPLAY * display = NULL;
ALLEGRO_COLOR BACKGROUND_COLOR;

ALLEGRO_FONT * font = NULL;
ALLEGRO_COLOR font_color;
ALLEGRO_EVENT_QUEUE * queue = NULL;


static inline void al_check(bool test, const char * description)
{
    if (test) return;
    fprintf(stderr, "ALLEGRO error in %s\n", description);
    exit(EXIT_ALLEGRO_ERROR);
}

static inline void allegro_init()
{
    al_check(al_init(), "al_init()");

    queue = al_create_event_queue();
    al_check(queue, "al_create_event_queue()");

    // Display
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR | ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
    display = al_create_display(DISPLAY_W, DISPLAY_H);
    al_check(display, "al_create_display()");
    al_register_event_source(queue, al_get_display_event_source(display));
    BACKGROUND_COLOR = al_map_rgba(0, 0, 0, 255);

    // Input
    al_check(al_install_keyboard(), "al_install_keyboard()");
    al_register_event_source(queue, al_get_keyboard_event_source());

    // Audio
    al_init_acodec_addon();
    al_check(al_install_audio(), "al_install_audio()");
    al_check(al_reserve_samples(0), "al_reserve_samples()");

    // Font
    al_check(al_init_font_addon(), "al_init_font_addon()");
    al_check(al_init_ttf_addon(), "al_init_ttf_addon()");
    font = al_load_ttf_font(FONT_NAME, FONT_SIZE, 0);
    font_color = al_map_rgba(255, 0, 0 , 255);

    // Image
    al_check(al_init_primitives_addon(), "al_init_primitives_addon()");
}

 ALLEGRO_CHANNEL_CONF allegro_channel_conf_with(size_t channels)
 {
    return (channels == 2 ? ALLEGRO_CHANNEL_CONF_2 : ALLEGRO_CHANNEL_CONF_1);
 }

static inline void allegro_print_text(const char * text, const float x, const float y, const int align)
{
    al_draw_text(font, font_color, x, y, align, text);
}

static inline ALLEGRO_BITMAP * allegro_load_bitamp(const char localDirectory[], const char filename[])
{
    ALLEGRO_PATH * path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
    al_append_path_component(path, localDirectory);
    al_set_path_filename(path, filename);
    DLOG("loaded bitmap: %s\n", al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP), NULL);
    ALLEGRO_BITMAP * bitmap = al_load_bitmap(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));
    al_destroy_path(path);
    return bitmap;
}

static inline void allegro_lock_read(ALLEGRO_BITMAP * bitmap)
{
    ALLEGRO_LOCKED_REGION * region = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
    if (region == NULL) {
        DLOG("allegro_lock_read: ERROR\n", NULL);
    }
}

static inline void allegro_unlock(ALLEGRO_BITMAP * bitmap)
{
    al_unlock_bitmap(bitmap);
}

static inline void allegro_free()
{
    al_uninstall_keyboard();
    al_uninstall_audio();
    al_destroy_font(font);
    al_shutdown_ttf_addon();
    al_shutdown_font_addon();
    al_shutdown_primitives_addon();
    al_destroy_display(display);
    al_destroy_event_queue(queue);
}

#endif

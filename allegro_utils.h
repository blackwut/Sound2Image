#ifndef ALLEGRO_UTIL_H
#define ALLEGRO_UTIL_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include "common.h"


ALLEGRO_DISPLAY * display = NULL;
ALLEGRO_EVENT_QUEUE * queue = NULL;

static inline void al_check(bool test, const char * description)
{
    if (test) return;
    fprintf(stderr, "ALLEGRO error in %s\n", description);
    exit(1);
}

static inline void allegro_init()
{
    al_check(al_init(), "al_init()");
    al_check(al_install_keyboard(), "al_install_keyboard()");

    queue = al_create_event_queue();
    al_check(queue, "al_create_event_queue()");

    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR | ALLEGRO_MEMORY_BITMAP | ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
    display = al_create_display(DISPLAY_W, DISPLAY_H);
    al_check(display, "al_create_display()");

    al_check(al_init_image_addon(), "al_init_image_addon()");
    al_check(al_init_primitives_addon(), "al_init_primitives_addon()");

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));

    al_clear_to_color(al_map_rgba(255, 255, 255, 255));
    al_flip_display();
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

static inline int allegro_screenshot(const char * destination_path, const char * gamename)
{
    ALLEGRO_PATH *path;
    char *filename, *filename_wp;
    struct tm *tmp;
    time_t t;
    unsigned int i;
    const char *path_cstr;

    if(!destination_path)
        path = al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
    else
        path = al_create_path_for_directory(destination_path);

    if(!path)
        return -1;

    if(!gamename) {
        if( !(gamename = al_get_app_name()) ) {
            al_destroy_path(path);
            return -2;
        }
    }

    t = time(0);
    tmp = localtime(&t);
    if(!tmp) {
        al_destroy_path(path);
        return -3;
    }

    // Length of gamename + length of "-YYYYMMDD" + length of maximum [a-z] + NULL terminator
    if ( !(filename_wp = filename = (char *)malloc(strlen(gamename) + 9 + 2 + 1)) ) {
        al_destroy_path(path);
        return -4;
    }

    strcpy(filename, gamename);
    // Skip to the '.' in the filename, or the end.
    for(; *filename_wp != '.' && *filename_wp != 0; ++filename_wp);

    *filename_wp++ = '-';
    if(strftime(filename_wp, 9, "%Y%m%d", tmp) != 8) {
        free(filename);
        al_destroy_path(path);
        return -5;
    }
    filename_wp += 8;

    for(i = 0; i < 26*26; ++i) {
        if(i > 25) {
            filename_wp[0] = (i / 26) + 'a';
            filename_wp[1] = (i % 26) + 'a';
            filename_wp[2] = 0;
        }
        else {
            filename_wp[0] = (i % 26) + 'a';
            filename_wp[1] = 0;
        }

        al_set_path_filename(path, filename);
        al_set_path_extension(path, ".png");
        path_cstr = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);

        if (al_filename_exists(path_cstr))
            continue;

        printf("path: %s\n", path_cstr);
        al_save_bitmap(path_cstr, al_get_target_bitmap());
        free(filename);
        al_destroy_path(path);
        return 0;
    }

    free(filename);
    al_destroy_path(path);

    return -6;
}

static inline void allegro_free()
{
    al_destroy_display(display);
    // al_destroy_timer(timer);
    al_destroy_event_queue(queue);
}

#endif

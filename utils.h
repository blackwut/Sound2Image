#ifndef UTILS_H
#define UTILS_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include "common.h"
#include "fft_audio.h"

/**
    Shapes
*/
#define M
#define THICKNESS_MULTIPLIER 8

#define SHAPE_ROUND_X(x) labs((x) *  3) % DISPLAY_W
#define SHAPE_ROUND_Y(y) labs((y) *  5) % DISPLAY_H
#define SHAPE_ROUND_W(x) labs((x) *  7) % DISPLAY_W
#define SHAPE_ROUND_H(y) labs((y) * 11) % DISPLAY_H

#define MAP_RGB(r, g, b) al_map_rgb((r) % 256, (g) % 256, (b) % 256)

#define LINE                0 // void al_draw_line            (float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness)
#define TRIANGLE            1 // void al_draw_rectangle       (float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness)
#define RECTANGLE           2 // void al_draw_ellipse         (float cx, float cy, float rx, float ry, ALLEGRO_COLOR color, float thickness)
#define ELLIPSE             3 // void al_draw_triangle        (float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR color, float thickness)
#define TRIANGLE_FILLED     4 // void al_draw_filled_triangle (float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR color)
#define RECTANGLE_FILLED    5 // void al_draw_filled_rectangle(float x1, float y1, float x2, float y2, ALLEGRO_COLOR color)
#define ELLIPSE_FILLED      6 // void al_draw_filled_ellipse  (float cx, float cy, float rx, float ry, ALLEGRO_COLOR color)

typedef struct shape {
    int type;
    float coords[6];
    ALLEGRO_COLOR color;
    float thickness;
} Shape;

// typedef struct shape_bmp {
//     ALLEGRO_BITMAP * bmp;
//     float cx;
//     float cy;
//     float dx;
//     float dy;
//     float xscale;
//     float yscale;
//     float angle;
// } Shape_bmp;

// static inline Shape_bmp * create_shape_bmp(const float x, const float y, const float w, const float h, const float scale, const float angle)
// {
//     Shape_bmp * shape = (Shape_bmp *)calloc(1, sizeof(Shape_bmp));
//     shape->bmp = al_create_bitmap((int)w, (int)h);
//     shape->cx = w / 2;
//     shape->cy = h / 2;
//     shape->dx = x;
//     shape->dy = y;
//     shape->xscale = scale;
//     shape->yscale = scale;
//     shape->angle = angle;

//     return shape;
// }

// static inline Shape_bmp * shape_bmp_line(const fft_audio_stats * stats)
// {
//     const long max          = (long)fabsf(stats->max);
//     const long power        = (long)fabsf(stats->power);
//     const long amplitude    = (long)fabsf(stats->amplitude);
//     const long RMS          = (long)fabsf(stats->RMS);
//     const long dB           = (long)fabsf(stats->dB);
//     const float phase       = stats->phase;

//     const long x1 = SHAPE_ROUND_X(max);
//     const long y1 = SHAPE_ROUND_Y(max);
//     const long x2 = SHAPE_ROUND_W(power);
//     const long y2 = SHAPE_ROUND_H(power);
//     const ALLEGRO_COLOR color = MAP_RGB(dB, amplitude, RMS);
//     const long thickness = fabsf(phase) * THICKNESS_MULTIPLIER;

//     const float x = MIN(x1, x2);
//     const float y = MIN(y1, y2);
//     const float w = MAX(x1, x2);
//     const float h = MAX(y1, y2);
//     const float scale = 1;
//     const float angle = 0;
//     DLOG("x: %f\ty: %f\tw: %f\th: %f\tscale: %f\tangle: %f\n", x, y, w, h, scale, angle);
//     Shape_bmp * shape = create_shape_bmp(x, y, w, h, scale, angle);
//     al_set_target_bitmap(shape->bmp);
//     al_clear_to_color(al_map_rgba(0, 0, 0, 0));
//     al_draw_line(0, 0, w, h, color, thickness);
//     al_set_target_bitmap(NULL);

//     return shape;
// }

static inline void draw(Shape * shape)
{
    const float x1 = shape->coords[0];
    const float x2 = shape->coords[1];
    const float x3 = shape->coords[2];
    const float x4 = shape->coords[3];
    const float x5 = shape->coords[4];
    const float x6 = shape->coords[5];

    switch (shape->type) {
        case LINE:
            al_draw_line(x1, x2, x3, x4, shape->color, shape->thickness);
            DLOG("LINE x1: %05.2f\ty1: %5.2f\tx2: %5.2f\ty2: %5.2f\n", x1, x2, x3, x4);
            break;
        case TRIANGLE:
            al_draw_triangle(x1, x2, x3, x4, x5, x6, shape->color, shape->thickness);
            DLOG("TRIA x1: %05.2f\ty1: %5.2f\tx2: %5.2f\ty2: %5.2f\tx3: %5.2f\ty3: %5.2f\n", x1, x2, x3, x4, x5, x6);
            break;
        case RECTANGLE:
            al_draw_rectangle(x1, x2, x3, x4, shape->color, shape->thickness);
            DLOG("RECT x1: %05.2f\ty1: %5.2f\tx2: %5.2f\ty2: %5.2f\t\n", x1, x2, x3, x4);
            break;
        case ELLIPSE:
            al_draw_ellipse(x1, x2, x3, x4, shape->color, shape->thickness);
            DLOG("ELLI cx: %05.2f\tcy: %5.2f\trx: %5.2f\try: %5.2f\n", x1, x2, x3, x4);
            break;
        case TRIANGLE_FILLED:
            al_draw_filled_triangle(x1, x2, x3, x4, x5, x6, shape->color);
            DLOG("FTRI x1: %05.2f\ty1: %5.2f\tx2: %5.2f\ty2: %5.2f\tx3: %5.2f\ty3: %5.2f\n", x1, x2, x3, x4, x5, x6);
            break;
        case RECTANGLE_FILLED:
            al_draw_filled_rectangle(x1, x2, x3, x4, shape->color);
            DLOG("FREC x1: %05.2f\ty1: %5.2f\tx2: %5.2f\ty2: %5.2f\t\n", x1, x2, x3, x4);
            break;
        case ELLIPSE_FILLED:
            al_draw_filled_ellipse(x1, x2, x3, x4, shape->color);
            DLOG("FELL x1: %05.2f\ty1: %5.2f\tx2: %5.2f\ty2: %5.2f\t\n", x1, x2, x3, x4);
            break;
        default:
            break;
        }
}

// void al_draw_line(float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness)
static inline void shape_line(Shape * shape, const fft_audio_stats * stats)
{
    //const long min          = (long)fabsf(stats->min);
    const long max          = (long)fabsf(stats->max);
    const long power        = (long)fabsf(stats->power);
    const long amplitude    = (long)fabsf(stats->amplitude);
    const long RMS          = (long)fabsf(stats->RMS);
    const long dB           = (long)fabsf(stats->dB);
    const float phase        = stats->phase;

    // DLOG("max: %ld\tpower: %ld\tamplitude: %ld\tRMS: %ld\tdB: %ld\tphase: %f\n", max, power, amplitude, RMS, dB, phase);

    const long x1 = SHAPE_ROUND_X(max);
    const long x2 = SHAPE_ROUND_Y(max);
    const long y1 = SHAPE_ROUND_W(power);
    const long y2 = SHAPE_ROUND_H(power);
    const ALLEGRO_COLOR color = MAP_RGB(dB, amplitude, RMS);
    const long thickness = fabsf(phase) * THICKNESS_MULTIPLIER;

    shape->type = LINE;
    shape->coords[0] = x1;
    shape->coords[1] = x2;
    shape->coords[2] = y1;
    shape->coords[3] = y2;
    shape->coords[4] = 0;
    shape->coords[5] = 0;
    shape->color = color;
    shape->thickness = thickness;
}

// void al_draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR color, float thickness)
static inline void shape_triangle(Shape * shape, const fft_audio_stats * stats)
{
    //const long min          = (long)fabsf(stats->min);
    const long max          = (long)fabsf(stats->max);
    const long power        = (long)fabsf(stats->power);
    const long amplitude    = (long)fabsf(stats->amplitude);
    const long RMS          = (long)fabsf(stats->RMS);
    const long dB           = (long)fabsf(stats->dB);
    const float phase        = stats->phase;

    // DLOG("max: %ld\tpower: %ld\tamplitude: %ld\tRMS: %ld\tdB: %ld\tphase: %f\n", max, power, amplitude, RMS, dB, phase);
    
    const long x1 = SHAPE_ROUND_X(power);
    const long y1 = SHAPE_ROUND_Y(power);
    const long x2 = SHAPE_ROUND_X(max);
    const long y2 = SHAPE_ROUND_Y(max);
    const long x3 = SHAPE_ROUND_X(amplitude);
    const long y3 = SHAPE_ROUND_Y(amplitude);
    const ALLEGRO_COLOR color = MAP_RGB(dB, amplitude, RMS);
    const long thickness = fabsf(phase) * THICKNESS_MULTIPLIER;

    shape->type = RECTANGLE;
    shape->coords[0] = x1;
    shape->coords[1] = y1;
    shape->coords[2] = x2;
    shape->coords[3] = y2;
    shape->coords[4] = x3;
    shape->coords[5] = y3;
    shape->color = color;
    shape->thickness = thickness;
}

// void al_draw_rectangle(float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness)
static inline void shape_rectangle(Shape * shape, const fft_audio_stats * stats)
{
    //const long min          = (long)fabsf(stats->min);
    const long max          = (long)fabsf(stats->max);
    const long power        = (long)fabsf(stats->power);
    const long amplitude    = (long)fabsf(stats->amplitude);
    const long RMS          = (long)fabsf(stats->RMS);
    const long dB           = (long)fabsf(stats->dB);
    const float phase        = stats->phase;

    // DLOG("max: %ld\tpower: %ld\tamplitude: %ld\tRMS: %ld\tdB: %ld\tphase: %f\n", max, power, amplitude, RMS, dB, phase);

    const long x1 = SHAPE_ROUND_X(max);
    const long y1 = SHAPE_ROUND_Y(max);
    const long x2 = SHAPE_ROUND_W(power);
    const long y2 = SHAPE_ROUND_H(power);
    const ALLEGRO_COLOR color = MAP_RGB(dB, amplitude, RMS);
    const long thickness = fabsf(phase) * THICKNESS_MULTIPLIER;

    shape->type = RECTANGLE;
    shape->coords[0] = x1;
    shape->coords[1] = y1;
    shape->coords[2] = x2;
    shape->coords[3] = y2;
    shape->coords[4] = 0;
    shape->coords[5] = 0;
    shape->color = color;
    shape->thickness = thickness;
}

// void al_draw_ellipse(float cx, float cy, float rx, float ry, ALLEGRO_COLOR color, float thickness)
static inline void shape_ellipse(Shape * shape, const fft_audio_stats * stats)
{
    //const long min          = (long)fabsf(stats->min);
    const long max          = (long)fabsf(stats->max);
    const long power        = (long)fabsf(stats->power);
    const long amplitude    = (long)fabsf(stats->amplitude);
    const long RMS          = (long)fabsf(stats->RMS);
    const long dB           = (long)fabsf(stats->dB);
    const float phase        = stats->phase;

    // DLOG("max: %ld\tpower: %ld\tamplitude: %ld\tRMS: %ld\tdB: %ld\tphase: %f\n", max, power, amplitude, RMS, dB, phase);

    const long rx = SHAPE_ROUND_W(max);
    const long ry = SHAPE_ROUND_H(max);
    const long cx = rx + SHAPE_ROUND_X(power);
    const long cy = ry + SHAPE_ROUND_Y(power);
    const ALLEGRO_COLOR color = MAP_RGB(dB, amplitude, RMS);
    const long thickness = fabsf(phase) * THICKNESS_MULTIPLIER;

    shape->type = ELLIPSE;
    shape->coords[0] = cx;
    shape->coords[1] = cy;
    shape->coords[2] = rx;
    shape->coords[3] = ry;
    shape->coords[4] = 0;
    shape->coords[5] = 0;
    shape->color = color;
    shape->thickness = thickness;
}

//void al_draw_filled_triangle(float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR color)
static inline void shape_filled_triangle(Shape * shape, const fft_audio_stats * stats)
{
    shape_triangle(shape, stats);
    shape->type = TRIANGLE_FILLED;
}

// void al_draw_filled_rectangle(float x1, float y1, float x2, float y2, ALLEGRO_COLOR color)
static inline void shape_filled_rectangle(Shape * shape, const fft_audio_stats * stats)
{
    shape_rectangle(shape, stats);
    shape->type = RECTANGLE_FILLED;
}

// void al_draw_filled_ellipse(float cx, float cy, float rx, float ry, ALLEGRO_COLOR color)
static inline void shape_filled_ellipse(Shape * shape, const fft_audio_stats * stats)
{
    shape_ellipse(shape, stats);
    shape->type = ELLIPSE_FILLED;
}


// Save a copy of the current target_bitmap (usually what's on the screen).
// The screenshot is placed in `destination_path`.
// The filename will follow the format "`gamename`-YYYYMMDD[a-z].png"
// Where [a-z] starts at 'a' and increases (towards 'z') to prevent duplicates
// on the same day.
// This filename format allows for easy time-based sorting in a file manager,
// even if the "Modified Date" or other file information is lost.
//
// Arguments:
// `destination_path` - Where to put the screenshot. If NULL, uses
//      ALLEGRO_USER_DOCUMENTS_PATH.
//
// `gamename` - The name of your game (only use path-valid characters).
//      If NULL, uses al_get_app_name().
//
//
// Returns:
// 0 on success, anything else on failure.


#endif

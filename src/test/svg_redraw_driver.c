#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <svgtiny.h>
typedef uint32_t colour;

#include "wisp/plot_style.h"
#include "wisp/plotters.h"
#include "wisp/utils/errors.h"

static bool svg_plot_diagram(const struct svgtiny_diagram *diagram, int x, int y, int width, int height,
    const struct rect *clip, const struct redraw_context *ctx, colour background_colour, colour current_color)
{
    (void)width;
    (void)height;
    (void)background_colour;
    (void)current_color;

    float transform[6];
    int px, py;
    unsigned int i;
    plot_font_style_t fstyle = *plot_style_font;
    plot_style_t pstyle;
    nserror res;

    assert(diagram);

    transform[0] = 1.0f;
    transform[1] = 0.0f;
    transform[2] = 0.0f;
    transform[3] = 1.0f;
    transform[4] = x;
    transform[5] = y;

#define BGR(c) (((svgtiny_RED((c))) | ((svgtiny_GREEN((c)) << 8)) | ((svgtiny_BLUE((c)) << 16))))

    for (i = 0; i != diagram->shape_count; i++) {
        if (diagram->shape[i].path) {
            if (diagram->shape[i].fill_gradient_type != svgtiny_GRADIENT_NONE &&
                ((diagram->shape[i].fill_gradient_type == svgtiny_GRADIENT_LINEAR && ctx->plot->linear_gradient != NULL) ||
                 (diagram->shape[i].fill_gradient_type == svgtiny_GRADIENT_RADIAL && ctx->plot->radial_gradient != NULL))) {
                struct gradient_stop *stops = malloc(diagram->shape[i].fill_grad_stop_count * sizeof(struct gradient_stop));
                if (stops != NULL) {
                    for (unsigned int s = 0; s < diagram->shape[i].fill_grad_stop_count; s++) {
                        svgtiny_colour c = diagram->shape[i].fill_grad_stops[s].color;
                        stops[s].color = (svgtiny_RED(c)) | (svgtiny_GREEN(c) << 8) | (svgtiny_BLUE(c) << 16);
                        stops[s].offset = diagram->shape[i].fill_grad_stops[s].offset;
                    }
                    if (diagram->shape[i].fill_gradient_type == svgtiny_GRADIENT_LINEAR) {
                        res = ctx->plot->linear_gradient(ctx, diagram->shape[i].path, diagram->shape[i].path_length, transform, diagram->shape[i].fill_grad_x1, diagram->shape[i].fill_grad_y1, diagram->shape[i].fill_grad_x2, diagram->shape[i].fill_grad_y2, stops, diagram->shape[i].fill_grad_stop_count);
                    } else {
                        res = ctx->plot->radial_gradient(ctx, diagram->shape[i].path, diagram->shape[i].path_length, transform, diagram->shape[i].fill_grad_x1, diagram->shape[i].fill_grad_y1, diagram->shape[i].fill_grad_x2, diagram->shape[i].fill_grad_y2, stops, diagram->shape[i].fill_grad_stop_count);
                    }
                    free(stops);
                    if (res != NSERROR_OK) return false;
                    continue;
                }
            }

            if (diagram->shape[i].stroke == svgtiny_TRANSPARENT) {
                pstyle.stroke_type = PLOT_OP_TYPE_NONE;
                pstyle.stroke_colour = NS_TRANSPARENT;
            } else {
                pstyle.stroke_type = PLOT_OP_TYPE_SOLID;
                pstyle.stroke_colour = BGR(diagram->shape[i].stroke);
            }
            pstyle.stroke_width = plot_style_int_to_fixed(diagram->shape[i].stroke_width);

            if (diagram->shape[i].fill == svgtiny_TRANSPARENT) {
                pstyle.fill_type = PLOT_OP_TYPE_NONE;
                pstyle.fill_colour = NS_TRANSPARENT;
            } else {
                pstyle.fill_type = PLOT_OP_TYPE_SOLID;
                pstyle.fill_colour = BGR(diagram->shape[i].fill);
            }

            res = ctx->plot->path(ctx, &pstyle, diagram->shape[i].path, diagram->shape[i].path_length, transform);
            if (res != NSERROR_OK) {
                return false;
            }
        } else if (diagram->shape[i].text) {
            px = (int)(diagram->shape[i].text_x) + transform[4];
            py = (int)(diagram->shape[i].text_y) + transform[5];

            fstyle.background = 0xffffff;
            fstyle.foreground = 0x000000;
            fstyle.size = (8 * PLOT_STYLE_SCALE);

            res = ctx->plot->text(ctx, &fstyle, px, py, diagram->shape[i].text, strlen(diagram->shape[i].text));
            if (res != NSERROR_OK) {
                return false;
            }
        }
    }

#undef BGR
    return true;
}

bool svg_redraw_diagram(struct svgtiny_diagram *diagram, int x, int y, int width, int height, const struct rect *clip,
    const struct redraw_context *ctx, colour background_colour, colour current_color)
{
    (void)clip;
    return svg_plot_diagram(diagram, x, y, width, height, clip, ctx, background_colour, current_color);
}

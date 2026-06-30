#ifndef WISP_WINDOWS_D2D_TYPES_H_
#define WISP_WINDOWS_D2D_TYPES_H_

struct d2d_path_command {
    int type; // Matches path_command enum
    float x1, y1, x2, y2, x3, y3;
};

#endif

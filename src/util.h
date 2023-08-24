#pragma once

#include <math.h>

struct Vec2
{
    int x;
    int y;
};


struct Vec2f
{
    float x;
    float y;
};

static inline int smin(int a, int b)
{
    return a > b ? b : a;
}

static inline int smax(int a, int b)
{
    return a > b ? a : b;
}

static inline float get_angle_radians(float x, float y)
{
    return atan2f(y, x);
}

static inline float get_angle_degrees(float x, float y)
{
    return atan2f(y, x) * 180 / 3.14159;
}
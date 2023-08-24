#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <libdragon.h>
#include <string.h>

#include "sm64_magnitude_test.h"
#include "range_test.h"
#include "drawing.h"
#include "text.h"
#include "colors.h"
#include "input.h"
#include "util.h"



const char *MAIN_TITLE_TEXT = "visualization of the deadzone\nL or R to change modes, B to clear\nA to hide dots, Z to hide values";
const char *MAGNITUDE_AND_ANGLE_TITLE_TEXT = "visualization of magnitude and angle vs raw values\nL or R to change modes, B to clear\nA to hide dots, Z to hide values";
const char *VARIABLES_TEXT = "rawStickX\nrawStickY\nnetStickX\nnetStickY\n\nmagnitude\nmagnitudeNoCap\n\nfinalStickX\nfinalStickY\nangle\nintendedMagnitude";
const int MAX_MAGNITUDE_TEST_MODE = 1;

enum MagnitudeTestMode
{
    MTM_MAIN,
    MTM_MAGNITUDE_AND_ANGLE,
};

void display_sm64_magnitude_test() {
    //initialize
    int dotCount = 0,
        lineHeight = 11,
        magnitudeTestMode = 0,
        historyLength = 512;
    bool showHistory = true;
    bool showVariables = true;
    text_set_line_height(lineHeight);
    display_context_t ctx;

    int f = dfs_open("/gfx/point_main.sprite");
    int sizeF = dfs_size(f);
    sprite_t *pointMain = malloc(sizeF);
    dfs_read(pointMain, sizeF, 1, f);
    dfs_close(f);

    int g = dfs_open("/gfx/point_red.sprite");
    int sizeG = dfs_size(g);
    sprite_t *pointMagnitudeAndAngle = malloc(sizeG);
    dfs_read(pointMagnitudeAndAngle, sizeG, 1, g);
    dfs_close(g);

    struct Vec2 rawHistory[historyLength];
    struct Vec2 magnitudeAndAngleHistory[historyLength];
    uint32_t rawHistoryColor = graphics_make_color(0, 192, 255, 255);
    uint32_t magnitudeAndAngleHistoryColor = graphics_make_color(255, 0, 0, 255);

    bool shouldContinue = true;
    while (shouldContinue) {
        // initialize
        while ((ctx = display_lock()) == 0) {}

        display_show(ctx);
        graphics_fill_screen(ctx, COLOR_BACKGROUND);
        graphics_set_color(COLOR_FOREGROUND, 0);
        if (magnitudeTestMode == MTM_MAIN) draw_deadzone(ctx);

        controller_scan();
        struct controller_data cdata = get_keys_pressed();
        char buf[256];


        // process values
        struct Vec2 rawStick = { cdata.c[0].x, cdata.c[0].y };
        struct Vec2 rawStickNoDeadzone = { cdata.c[0].x, cdata.c[0].y };
        struct Vec2 netStick = { 0, 0 };

        if (rawStickNoDeadzone.x < 8 && rawStickNoDeadzone.x > -8) {
            rawStickNoDeadzone.x = 0;
        }
        if (rawStickNoDeadzone.y < 8 && rawStickNoDeadzone.y > -8) {
            rawStickNoDeadzone.y = 0;
        }

        if (rawStick.x <= -8) {
            netStick.x = rawStick.x + 6;
        }
        else if (rawStick.x >= 8) {
            netStick.x = rawStick.x - 6;
        }
        
        if (rawStick.y <= -8) {
            netStick.y = rawStick.y + 6;
        }
        else if (rawStick.y >= 8) {
            netStick.y = rawStick.y - 6;
        }

        float magnitudeNoCap = sqrt(pow(netStick.x, 2) + pow(netStick.y, 2));
        float magnitude = magnitudeNoCap;

        struct Vec2f finalStick = { netStick.x, netStick.y };
        
        if (magnitudeNoCap > 64)
        {
            finalStick.x = netStick.x * 64 / magnitudeNoCap;
            finalStick.y = netStick.y * 64 / magnitudeNoCap;
            magnitude = 64.0f;
        }

        float angleDegrees = get_angle_degrees(finalStick.x, finalStick.y);
        float angleRadians = get_angle_radians(finalStick.x, finalStick.y);
        float intendedMagnitude = pow(magnitude / 64, 2) * 64 / 2;

        struct Vec2 magnitudeAndAngleCalculation = { 0, 0 };
        magnitudeAndAngleCalculation.x = round(magnitude * cos(angleRadians));
        magnitudeAndAngleCalculation.y = round(magnitude * sin(angleRadians));


        // draw variables text
        if (showVariables)
        {
            snprintf(buf, sizeof(buf), VARIABLES_TEXT);
            text_set_font(FONT_MEDIUM);
            text_draw(ctx, 220, 60 - lineHeight, buf, ALIGN_LEFT);

            text_set_font(FONT_BOLD);
            snprintf(buf, sizeof(buf), "%3d\n%3d\n%3d\n%3d\n\n%.2f\n%.2f\n\n%.2f\n%.2f\n%.2f\n%.2f", rawStick.x, rawStick.y, netStick.x, netStick.y, magnitude, magnitudeNoCap, finalStick.x, finalStick.y, angleDegrees, intendedMagnitude);
            text_draw(ctx, 212, 60 - lineHeight, buf, ALIGN_RIGHT);
        }

        int rawCursorX = 0,
            rawCursorY = 0,
            magnitudeAndAngleCursorX = 0,
            magnitudeAndAngleCursorY = 0;
        

        // draw graphs/history/cursors based on mode
        bool historyUpdate = false;
        bool hasChanged = rawStick.x != rawHistory[0].x || rawStick.y != rawHistory[0].y;

        if (magnitudeTestMode == MTM_MAIN) {
            bool isNotInDeadzone = abs(rawStick.x) >= 8 || abs(rawStick.y) >= 8;
            if (showHistory && hasChanged && isNotInDeadzone) {
                historyUpdate = true;
                if (dotCount < historyLength - 1) {
                    dotCount++;
                }
                rawHistory[0] = rawStickNoDeadzone;
            }
            rawCursorX = smax(0, smin(320, rawStickNoDeadzone.x + 158));
            rawCursorY = smax(0, smin(240, (rawStickNoDeadzone.y * -1) + 118));
        }
        else {
            if (showHistory && hasChanged) {
                historyUpdate = true;
                if (dotCount < historyLength - 1) {
                    dotCount++;
                }
                rawHistory[0] = rawStick;
                magnitudeAndAngleHistory[0] = magnitudeAndAngleCalculation;
            }
            rawCursorX = smax(0, smin(320, rawStick.x + 158));
            rawCursorY = smax(0, smin(240, (rawStick.y * -1) + 118));
            magnitudeAndAngleCursorX = smax(0, smin(320, magnitudeAndAngleCalculation.x + 158));
            magnitudeAndAngleCursorY = smax(0, smin(240, (magnitudeAndAngleCalculation.y * -1) + 118));
            draw_center_cross(ctx, 160);
            
            for (int i = dotCount; i > 0 && showHistory; i--) {
                if (historyUpdate) magnitudeAndAngleHistory[i] = magnitudeAndAngleHistory[i - 1];
                int x = smax(0, smin(320, magnitudeAndAngleHistory[i].x + 160));
                int y = smax(0, smin(240, (magnitudeAndAngleHistory[i].y * -1) + 120));
                graphics_draw_pixel(ctx, x, y, magnitudeAndAngleHistoryColor);
            }
            
            graphics_draw_sprite(ctx, magnitudeAndAngleCursorX, magnitudeAndAngleCursorY, pointMagnitudeAndAngle);
        }
        
        for (int i = dotCount; i > 0 && showHistory; i--) {
            if (historyUpdate) rawHistory[i] = rawHistory[i - 1];
            int x = smax(0, smin(320, rawHistory[i].x + 160));
            int y = smax(0, smin(240, (rawHistory[i].y * -1) + 120));
            graphics_draw_pixel(ctx, x, y, rawHistoryColor);
        }
        graphics_draw_sprite(ctx, rawCursorX, rawCursorY, pointMain);


        // draw the text
        if (netStick.x == 0 && netStick.y == 0)
        {
            text_set_font(FONT_MEDIUM);
            if (magnitudeTestMode == MTM_MAIN) {
                snprintf(buf, sizeof(buf), "%s", MAIN_TITLE_TEXT);
            }
            else {
                snprintf(buf, sizeof(buf), "%s", MAGNITUDE_AND_ANGLE_TITLE_TEXT);
            }
            text_draw(ctx, 160, 15, buf, ALIGN_CENTER);

            text_set_font(FONT_MEDIUM);
            graphics_set_color(graphics_make_color(128, 128, 128, 255), 0);
            text_draw(ctx, 320 - 16, 213, REPO_URL, ALIGN_RIGHT);
        }


        // process buttons
        if (cdata.c[0].start) {
            shouldContinue = false;
        }

        cdata = get_keys_down_filtered();
        if (cdata.c[0].A) {
            showHistory = !showHistory;
        }

        if (cdata.c[0].Z) {
            showVariables = !showVariables;
        }


        if (cdata.c[0].B) {
            dotCount = 0;
        }

        if (cdata.c[0].L) {
            magnitudeTestMode--;
            dotCount = 0;

            if (magnitudeTestMode < 0)
            {
                magnitudeTestMode = MAX_MAGNITUDE_TEST_MODE;
            }
        }

        if (cdata.c[0].R) {
            magnitudeTestMode++;
            dotCount = 0;

            if (magnitudeTestMode > MAX_MAGNITUDE_TEST_MODE)
            {
                magnitudeTestMode = 0;
            }
        }
    }

    free(pointMain);
    free(pointMagnitudeAndAngle);
}

void draw_deadzone(display_context_t ctx) {
    uint32_t c_red = graphics_make_color(255, 0, 0, 255);

    draw_aa_line(ctx, 153, 0, 153, 239, c_red);
    draw_aa_line(ctx, 167, 0, 167, 239, c_red);

    draw_aa_line(ctx, 0, 113, 319, 113, c_red);
    draw_aa_line(ctx, 0, 127, 319, 127, c_red);
}
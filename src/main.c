#include "raylib.h"
#include <stdio.h>

const int SIDE = 50;
const int DISC_RADIUS = SIDE / 2;
const int MAX_DISCS = 7;
const int SCREEN_WIDTH = SIDE * MAX_DISCS;
const int SCREEN_HEIGHT = 800;

int cubes[MAX_DISCS][MAX_DISCS];
Color colors[MAX_DISCS + 1];

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "DROP7");
    SetWindowPosition(0, 0);
    SetWindowFocused();
    SetTargetFPS(60);
    colors[0] = BLACK;
    colors[1] = RED;
    colors[2] = BLUE;
    colors[3] = GREEN;
    colors[4] = YELLOW;
    colors[5] = ORANGE;
    colors[6] = PURPLE;
    colors[7] = PINK;
    for (int i = 0; i < MAX_DISCS; i++) {
        for (int j = 0; j < MAX_DISCS; j++) {
            cubes[i][j] = GetRandomValue(1, MAX_DISCS);
        }
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        if (IsKeyPressed(KEY_P)) {
            for (int i = 0; i < MAX_DISCS; i++) {
                for (int j = 0; j < MAX_DISCS; j++) {
                    printf("%d%d\n", i, j);
                }
            }
        }
        // ROWS
        for (int i = 0; i < MAX_DISCS; i++) {
            int iOffset = i + 1;
            // COLUMNS
            for (int j = 0; j < MAX_DISCS; j++) {
                int jOffset = j + 1;
                int colorIndex = cubes[i][j];
                DrawCircle(iOffset * SIDE - DISC_RADIUS, jOffset * SIDE - DISC_RADIUS, DISC_RADIUS, colors[colorIndex]);
            }
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

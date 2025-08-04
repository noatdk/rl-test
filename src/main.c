#include "raylib.h"
#include <stdio.h>

#define SIDE 50
#define DISC_RADIUS SIDE / 2
#define MAX_DISCS 7
#define SCREEN_WIDTH SIDE *MAX_DISCS
#define SCREEN_HEIGHT 800

typedef struct Disc {
    int number;
    int state;
} Disc;

typedef struct Row {
    Disc discs[MAX_DISCS];
} Row;

Row rows[MAX_DISCS];
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
            rows[i].discs[j].number = GetRandomValue(1, MAX_DISCS);
            rows[i].discs[j].state = 0;
        }
    }

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            printf("click at %f\n", GetTime());
        }
        BeginDrawing();
        ClearBackground(BLACK);
        if (IsKeyPressed(KEY_P)) {
        }
        // ROWS
        for (int i = 0; i < MAX_DISCS; i++) {
            int iOffset = i + 1;
            // COLUMNS
            for (int j = 0; j < MAX_DISCS; j++) {
                int jOffset = j + 1;
                int colorIndex = rows[i].discs[j].number;
                DrawCircle(iOffset * SIDE - DISC_RADIUS, jOffset * SIDE - DISC_RADIUS, DISC_RADIUS, colors[colorIndex]);
            }
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

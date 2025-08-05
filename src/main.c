#include "raylib.h"
#include <stdio.h>
#include <threads.h>

#define SIDE 100
#define DISC_RADIUS SIDE / 2
#define MAX_DISCS 7
#define BACKGROUND GRAY
#define SCREEN_WIDTH SIDE *MAX_DISCS
#define SCREEN_HEIGHT (SIDE) * MAX_DISCS

typedef struct Disc {
    int number;
    int state;
    int colStack;
    int rowStack;
} Disc;

typedef struct Col {
    Disc discs[MAX_DISCS];
} Col;

typedef enum {
    INTACT,
    HALF_BROKEN,
    BROKEN,
    COUNT,
} DiscState;

Col cols[MAX_DISCS];
Color colors[MAX_DISCS + 1];

bool isBusting(Disc disc)
{
    return disc.number == disc.rowStack || disc.number == disc.colStack;
}

void calculateStacks()
{
    int spanMax = 0;
    int colMax = 0;
    for (int i = 0; i < MAX_DISCS; i++) {
        for (int j = 0; j < MAX_DISCS; j++) {
            if (cols[i].discs[j].number != 0) {
                colMax++;
            }
        }
        for (int j = 0; j < MAX_DISCS; j++) {
            if (cols[i].discs[j].number != 0) {
                cols[i].discs[j].colStack = colMax;
            }
        }
        colMax = 0;
    }
    for (int j = 0; j < MAX_DISCS; j++) {
        for (int i = 0; i < MAX_DISCS; i++) {
            if (cols[i].discs[j].number != 0) {
                spanMax++;
                for (int newSpanMax = spanMax; newSpanMax > 0; newSpanMax--) {
                    cols[i + 1 - newSpanMax].discs[j].rowStack = spanMax;
                }
            } else {
                spanMax = 0;
            }
        }
        spanMax = 0;
    }
}

void randomizeDiscs()
{
    // populate each columns with random amount of discs
    for (int i = 0; i < MAX_DISCS; i++) {
        for (int j = 0; j < MAX_DISCS; j++) {
            // 3 * j because we want the possiblity to rise as we get closer to the bottom
            if ((j > 0 && (cols[i].discs[j - 1].number == 1) || GetRandomValue(0, 99) < 3 * j)) {
                cols[i].discs[j].number = 1;
            } else {
                cols[i].discs[j].number = 0;
            }
        }
    }

    // calculate the horizontal and vertical stacks, and then assign new value
    // that is neither
    calculateStacks();
    for (int i = 0; i < MAX_DISCS; i++) {
        for (int j = 0; j < MAX_DISCS; j++) {
            Disc disc = cols[i].discs[j];
            if (disc.number == 1) {
                cols[i].discs[j].number = GetRandomValue(1, MAX_DISCS);
                while (isBusting(cols[i].discs[j])) {
                    cols[i].discs[j].number = GetRandomValue(1, MAX_DISCS);
                }
            }
        }
    }
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "DROP7");
    SetWindowPosition(0, 0);
    SetWindowFocused();
    SetTargetFPS(60);
    colors[0] = BACKGROUND;
    colors[1] = GREEN;
    colors[2] = YELLOW;
    colors[3] = ORANGE;
    colors[4] = RED;
    colors[5] = PINK;
    colors[6] = SKYBLUE;
    colors[7] = BLUE;
    randomizeDiscs();

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            printf("click at %f\n", GetTime());
        }
        BeginDrawing();
        ClearBackground(BACKGROUND);
        if (IsKeyPressed(KEY_P)) {
            randomizeDiscs();
        }
        // COLUMNS
        for (int i = 0; i < MAX_DISCS; i++) {
            int iOffset = i + 1;
            // ROWS
            for (int j = 0; j < MAX_DISCS; j++) {
                int jOffset = j + 1;
                Disc disc = cols[i].discs[j];
                Color color = colors[disc.number];
                // if (disc.state == INTACT) {
                //     color = WHITE;
                // }
                DrawCircle(iOffset * SIDE - DISC_RADIUS, jOffset * SIDE - DISC_RADIUS, DISC_RADIUS, color);
                if (disc.number != 0) {
                    // DrawText(TextFormat("x=%d:y=%d\n%d:%d:%d", iOffset, jOffset, disc.colStack, disc.rowStack, disc.number), iOffset * SIDE - DISC_RADIUS - 15, jOffset * SIDE - DISC_RADIUS - 12, 15, BLACK);
                    DrawText(TextFormat("%d", disc.number), iOffset * SIDE - DISC_RADIUS * 1.3, jOffset * SIDE - DISC_RADIUS * 1.4, DISC_RADIUS, WHITE);
                }
            }
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

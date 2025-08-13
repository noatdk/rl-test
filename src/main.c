#include "raylib.h"
#include <stdio.h>

#define SIDE 100
#define DISC_RADIUS SIDE / 2
#define RADIUS_MODIFIER 0.96
#define MAX_DISCS 7
#define MAX_DISCS_AND_NEXT_ROW 8
#define NEXT_DISC_ROW 8
#define ORB_PER_LVL 5
#define ORB_SIDE SIDE / 3
#define ORB_RADIUS ORB_SIDE / 2
#define BACKGROUND DARKBLUE
#define UI_BACKGROUND LIGHTGRAY
#define SCREEN_WIDTH SIDE *MAX_DISCS
#define SCREEN_HEIGHT (SIDE) * (MAX_DISCS + 2)
#define REALLYDARKBLUE CLITERAL(Color){0, 29, 152, 128}
#define max(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b;      \
    })

#define min(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a < _b ? _a : _b;      \
    })

typedef struct Disc {
    int number;
    int state;
    int colStack;
    int rowStack;
    float targetY;
    float currentY;
    float initialDistance;
    float popProgress;
} Disc;

typedef struct Col {
    Disc discs[MAX_DISCS + 1];
    Disc newDiscs[MAX_DISCS + 1];
} Col;

typedef enum {
    INTACT,
    HALF_BROKEN,
    BROKEN,
    DISC_STATE_COUNT,
} DiscState;

typedef enum {
    INPUT_AWAIT,
    LOWERING,
    RAISING,
    POPPING,
    BREAKING,
    GAME_OVER,
    GAME_STATE_COUNT,
} GameState;

Col cols[MAX_DISCS];
Color colors[MAX_DISCS + 1];
Disc nextDisc;
int mode = INPUT_AWAIT;
int level = 1;
int orbs = 0;

bool isBusting(Disc disc)
{
    return disc.state == BROKEN && (disc.number == disc.rowStack || disc.number == disc.colStack);
}

int randomDiscNum()
{
    return GetRandomValue(1, MAX_DISCS);
}

float getCenter(int nth)
{
    return nth * SIDE - (float)DISC_RADIUS;
}

Disc randomDisc()
{
    Disc disc = {};
    disc.state = BROKEN;
    disc.number = randomDiscNum();
    return disc;
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

void swapDiscArrays(bool onlyClears)
{
    for (int i = 0; i < MAX_DISCS; i++) {
        for (int j = 0; j < MAX_DISCS_AND_NEXT_ROW; j++) {
            if (!onlyClears) {
                cols[i].discs[j] = cols[i].newDiscs[j];
            }
            cols[i].newDiscs[j] = (Disc){0};
        }
    }
}

void calculateDisplacements()
{
    swapDiscArrays(true);
    for (int i = 0; i < MAX_DISCS; i++) {
        // the bottom row doesnt have to move, so MAX_DISCS-2
        for (int j = MAX_DISCS - 1; j >= 0; j--) {
            Disc disc = cols[i].discs[j];
            int nextPlacement = j;
            if (disc.number == 0) {
                continue;
            }
            while (j != MAX_DISCS - 1 && cols[i].newDiscs[nextPlacement + 1].number == 0 && nextPlacement < MAX_DISCS - 1) {
                nextPlacement++;
            }
            cols[i].discs[j].currentY = getCenter(j + 1);
            cols[i].discs[j].targetY = getCenter(nextPlacement + 1);
            cols[i].discs[j].initialDistance = cols[i].discs[j].targetY - cols[i].discs[j].currentY;
            cols[i].newDiscs[nextPlacement] = cols[i].discs[j];
            cols[i].newDiscs[nextPlacement].currentY = cols[i].discs[j].targetY;
        }
    }
}

void raiseDiscs()
{
    swapDiscArrays(true);
    for (int i = 0; i < MAX_DISCS; i++) {
        for (int j = 0; j < MAX_DISCS_AND_NEXT_ROW; j++) {
            Disc disc = cols[i].discs[j];
            if (j == 0) {
                if (disc.number != 0) {
                    mode = GAME_OVER;
                    return;
                }
                continue;
            } else if (j == MAX_DISCS) {
                cols[i].discs[j] = randomDisc();
                cols[i].discs[j].state = INTACT;
            }
            cols[i].discs[j].currentY = getCenter(j + 1);
            cols[i].discs[j].targetY = getCenter(j);
            cols[i].newDiscs[j - 1] = cols[i].discs[j];
            cols[i].newDiscs[j - 1].currentY = cols[i].discs[j].targetY;
        }
    }
    mode = RAISING;
}

void collateDamage(int x, int y)
{
    if (y != 0) {
        // UP
        cols[x].discs[y - 1].state = min(cols[x].discs[y - 1].state + 1, BROKEN);
    }
    if (y != MAX_DISCS - 1) {
        // DOWN
        cols[x].discs[y + 1].state = min(cols[x].discs[y + 1].state + 1, BROKEN);
    }
    if (x != 0) {
        // LEFT
        cols[x - 1].discs[y].state = min(cols[x - 1].discs[y].state + 1, BROKEN);
    }
    if (x != MAX_DISCS - 1) {
        // RIGHT
        cols[x + 1].discs[y].state = min(cols[x + 1].discs[y].state + 1, BROKEN);
    }
}

void breakDiscs()
{
    int breakingDiscs = 0;
    for (int j = 0; j < MAX_DISCS; j++) {
        for (int i = 0; i < MAX_DISCS; i++) {
            Disc disc = cols[i].discs[j];
            if (disc.number != 0 && isBusting(disc)) {
                breakingDiscs++;
                cols[i].discs[j].popProgress = 0.1;
            }
        }
    }
    if (breakingDiscs == 0) {
        if (orbs == ORB_PER_LVL) {
            raiseDiscs();
            level++;
            orbs = 0;
            return;
        }
        mode = INPUT_AWAIT;
    } else {
        mode = POPPING;
    }
}

void updatePopProgress(float dt)
{
    int poppingDiscs = 0;
    for (int j = 0; j < MAX_DISCS; j++) {
        for (int i = 0; i < MAX_DISCS; i++) {
            Disc disc = cols[i].discs[j];
            if (disc.popProgress != 0) {
                poppingDiscs++;
                cols[i].discs[j].popProgress += 2 * dt;
            }
            if (cols[i].discs[j].popProgress >= 0.5) {
                poppingDiscs--;
                cols[i].discs[j].number = 0;
                collateDamage(i, j);
            }
        }
    }
    if (poppingDiscs == 0) {
        calculateDisplacements();
        mode = LOWERING;
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
            if (disc.number != 1) {
                continue;
            }
            cols[i].discs[j].number = randomDiscNum();
            cols[i].discs[j].state = BROKEN;
            cols[i].discs[j].currentY = getCenter(j + 1);
            cols[i].discs[j].targetY = getCenter(j + 1);
            while (isBusting(cols[i].discs[j])) {
                cols[i].discs[j].number = randomDiscNum();
            }
        }
    }
}

void drawGrids()
{
    for (int i = 0; i < MAX_DISCS; i++) {
        DrawLineEx((Vector2){i * SIDE, 0 * SIDE}, (Vector2){i * SIDE, MAX_DISCS * SIDE}, 2, WHITE);
    }
    for (int j = 0; j < MAX_DISCS_AND_NEXT_ROW; j++) {
        DrawLineEx((Vector2){0 * SIDE, j * SIDE}, (Vector2){MAX_DISCS * SIDE, j * SIDE}, 2, WHITE);
    }
}

void drawDiscs()
{
    // COLUMNS
    for (int i = 0; i < MAX_DISCS; i++) {
        int iOffset = i + 1;
        // ROWS
        for (int j = 0; j < MAX_DISCS_AND_NEXT_ROW; j++) {
            int jOffset = j + 1;
            Disc disc = cols[i].discs[j];
            if (disc.number == 0) {
                continue;
            }
            Color color = colors[disc.number];
            if (disc.state != BROKEN) {
                color = WHITE;
            }
            // 0.99 so the grids dont look weird
            DrawCircle(getCenter(iOffset), disc.currentY, (float)DISC_RADIUS * (RADIUS_MODIFIER + disc.popProgress), color);
            // DrawText(TextFormat("x=%d:y=%d\n%d:%d:%d", iOffset, jOffset, disc.colStack, disc.rowStack, disc.number), iOffset * SIDE - DISC_RADIUS - 15, jOffset * SIDE - DISC_RADIUS - 12, 15, BLACK);
            if (disc.state != BROKEN) {
                DrawCircle(getCenter(iOffset), disc.currentY, (float)DISC_RADIUS * (0.8), BLACK);
                DrawCircle(getCenter(iOffset), disc.currentY, (float)DISC_RADIUS * (0.7), color);
            } else {
                DrawText(TextFormat("%d", disc.number), iOffset * SIDE - DISC_RADIUS, disc.currentY, (float)DISC_RADIUS * (RADIUS_MODIFIER + disc.popProgress), WHITE);
            }
            if (disc.state == HALF_BROKEN) {
                DrawText(TextFormat("~", disc.number, disc.popProgress), iOffset * SIDE - DISC_RADIUS, disc.currentY, (float)DISC_RADIUS * 1, BLACK);
            }
        }
    }
}

void drawUI()
{
    // DrawText(TextFormat("mode: %d", mode), 0, 0, 10, WHITE);
    DrawRectangleV((Vector2){0, SCREEN_HEIGHT - 2 * SIDE}, (Vector2){SCREEN_WIDTH, SCREEN_HEIGHT}, UI_BACKGROUND);
    // draw next disc
    DrawCircle(getCenter(NEXT_DISC_ROW / 2), getCenter(NEXT_DISC_ROW + 1), (float)DISC_RADIUS * RADIUS_MODIFIER, colors[nextDisc.number]);
    DrawText(TextFormat("%d", nextDisc.number), getCenter(NEXT_DISC_ROW / 2), getCenter(NEXT_DISC_ROW + 1), (float)DISC_RADIUS * RADIUS_MODIFIER, WHITE);

    // draw orbs
    for (int i = 0; i < ORB_PER_LVL; i++) {
        DrawCircle((i + 1) * ORB_SIDE - ORB_RADIUS, getCenter(NEXT_DISC_ROW + 1), (float)ORB_RADIUS * 0.99, REALLYDARKBLUE);
        if (ORB_PER_LVL - i <= orbs) {
            DrawCircle((i + 1) * ORB_SIDE - ORB_RADIUS, getCenter(NEXT_DISC_ROW + 1), (float)ORB_RADIUS * 0.88, WHITE);
        }
    }
}

void gravitate(float dt, bool upwards)
{
    int movingDiscs = 0;
    for (int i = 0; i < MAX_DISCS; i++) {
        for (int j = 0; j < MAX_DISCS_AND_NEXT_ROW; j++) {
            if (upwards) {
                Disc disc = cols[i].discs[j];
                if (disc.number == 0 || !(disc.targetY < disc.currentY)) {
                    continue;
                }
                movingDiscs++;
                float newCurrentY = disc.currentY - dt * SIDE * 5;
                if (newCurrentY <= disc.targetY) {
                    newCurrentY = disc.targetY;
                }
                cols[i].discs[j].currentY = newCurrentY;
            } else {
                Disc disc = cols[i].discs[j];
                if (disc.number == 0 || !(disc.targetY > disc.currentY)) {
                    continue;
                }
                movingDiscs++;
                float newCurrentY = disc.currentY + dt * disc.initialDistance + 10;
                if (newCurrentY >= disc.targetY) {
                    newCurrentY = disc.targetY;
                }
                cols[i].discs[j].currentY = newCurrentY;
            }
        }
    }

    if (movingDiscs == 0) {
        swapDiscArrays(false);
        mode = BREAKING;
    }
}

void dropNextDisc()
{
    Vector2 mousePos = GetMousePosition();
    int x = mousePos.x / SIDE;
    int y = 0;
    if (cols[x].discs[y].number != 0) {
        return;
    }
    cols[x].discs[y] = nextDisc;
    cols[x].discs[y].currentY = getCenter(y + 1);
    orbs++;
    nextDisc = randomDisc();
    calculateDisplacements();
    mode = LOWERING;
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "DROP7");
    SetWindowPosition(0, 0);
    SetWindowFocused();
    SetTargetFPS(60);
    colors[0] = BACKGROUND;
    colors[1] = LIME;
    colors[2] = GOLD;
    colors[3] = ORANGE;
    colors[4] = MAROON;
    colors[5] = PINK;
    colors[6] = SKYBLUE;
    colors[7] = BLUE;

    randomizeDiscs();
    nextDisc = randomDisc();

    while (!WindowShouldClose()) {
        // UPDATE
        {
            float dt = GetFrameTime();
            if (IsKeyPressed(KEY_P)) {
                int mode = INPUT_AWAIT;
                int level = 1;
                int orbs = 0;
                printf("randomizing\n");
                randomizeDiscs();
            }

            // if (IsKeyPressed(KEY_K)) {
            //     printf("raising\n");
            //     raiseDiscs();
            // }

            if (mode == INPUT_AWAIT && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                dropNextDisc();
            }

            if (mode == LOWERING) {
                gravitate(dt, false);
            } else if (mode == RAISING) {
                gravitate(dt, true);
            }

            if (mode == BREAKING) {
                calculateStacks();
                breakDiscs();
            }

            if (mode == POPPING) {
                updatePopProgress(dt);
            }
        }

        // DRAWING
        BeginDrawing();
        {
            ClearBackground(BACKGROUND);
            drawGrids();
            drawDiscs();
            drawUI();
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

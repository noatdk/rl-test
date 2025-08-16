#include "raylib.h"
#include <locale.h>
#include <stdio.h>
#include <threads.h>

#define SIDE 100
#define DISC_RADIUS SIDE / 2
#define RADIUS_MODIFIER 0.96
#define MAX_DISCS 7
#define MAX_DISCS_AND_NEXT_ROW 8
#define NEXT_DISC_ROW 8
#define ORB_PER_LVL 5
#define ORB_SIDE SIDE / 3
#define ORB_RADIUS ORB_SIDE / 2
#define LEVEL_UP_BONUS 17000
#define LEVEL_UP_INDICATOR_TIME 2
#define MAX_CHAIN 30
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
    int score;
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
const int scoreMultiplierPerChain[MAX_CHAIN + 1] = {0,
                                                    7, 39, 109, 224, 391, 617, 907, 1267, 1701, 2213, 2809, 3491, 4265, 5133, 6099, 7168, 8341, 9622, 11014, 12521, 14146, 15891, 17758, 19752, 21875, 24128, 26515, 29039, 31702, 34506};
int mode = INPUT_AWAIT;
int level = 1;
int orbs = 0;
int score = 0;
int chain = 0;
float lastLevelUp = 0;
float restartTime = 0;

bool isBusting(Disc disc)
{
    return disc.state == BROKEN && (disc.number == disc.rowStack || disc.number == disc.colStack);
}

int randomDiscNum()
{
    return GetRandomValue(1, MAX_DISCS);
}

float getCenter(float nth)
{
    return nth * (float)SIDE - (float)DISC_RADIUS;
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
    score += LEVEL_UP_BONUS;
    lastLevelUp = GetTime();
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
                cols[i].discs[j].score = scoreMultiplierPerChain[min(chain + 1, MAX_CHAIN)];
            }
        }
    }
    if (breakingDiscs == 0) {
        chain = 0;
        if (orbs == ORB_PER_LVL) {
            raiseDiscs();
            level++;
            orbs = 0;
            return;
        }
        mode = INPUT_AWAIT;
    } else {
        chain++;
        chain = min(chain, MAX_CHAIN);
        score += breakingDiscs * scoreMultiplierPerChain[chain];
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
                cols[i].discs[j].popProgress += 1.5 * dt;
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
    Color scoreIncrementColor = colors[GetRandomValue(1, MAX_DISCS)];
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
            DrawCircle(getCenter(iOffset), disc.currentY, (float)DISC_RADIUS * (RADIUS_MODIFIER + disc.popProgress), color);
            if (disc.score != 0) {
                DrawText(TextFormat("+%d", disc.score), getCenter(iOffset) - 50, disc.currentY - 100, (float)DISC_RADIUS, scoreIncrementColor);
            }
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
        DrawCircle((i + 1) * ORB_SIDE - ORB_RADIUS + 20, getCenter(NEXT_DISC_ROW + 1), (float)ORB_RADIUS * 0.99, REALLYDARKBLUE);
        if (ORB_PER_LVL - i <= orbs) {
            DrawCircle((i + 1) * ORB_SIDE - ORB_RADIUS + 20, getCenter(NEXT_DISC_ROW + 1), (float)ORB_RADIUS * 0.88, WHITE);
        }
    }
    DrawText(TextFormat("Level: %d", level), ORB_SIDE - ORB_RADIUS, getCenter(NEXT_DISC_ROW + 0.3), (float)DISC_RADIUS, REALLYDARKBLUE);
    DrawText(TextFormat("Score: %'dpts", score), ORB_SIDE - ORB_RADIUS, getCenter(NEXT_DISC_ROW - 0.4), (float)DISC_RADIUS, REALLYDARKBLUE);
    if (lastLevelUp != 0 && GetTime() - LEVEL_UP_INDICATOR_TIME < lastLevelUp) {
        DrawText(TextFormat("LEVEL UP! +%d", LEVEL_UP_BONUS), getCenter(MAX_DISCS / 2 - 1), getCenter(1), (float)DISC_RADIUS, WHITE);
    }
    if (chain > 1) {
        Color color = colors[GetRandomValue(1, MAX_DISCS)];
        DrawText(TextFormat("%dx", chain), getCenter(MAX_DISCS - 1), getCenter(MAX_DISCS_AND_NEXT_ROW), (float)DISC_RADIUS * 2, color);
    }
    if (mode == GAME_OVER) {
        DrawText(TextFormat("GAME OVER"), getCenter(1), getCenter(1), (float)DISC_RADIUS * 2, WHITE);
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
    setlocale(LC_NUMERIC, "");
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
            if (restartTime > 0 && GetTime() - restartTime < 1) {
                continue;
            } else {
                restartTime = 0;
            }
            float dt = GetFrameTime();
            if (mode == GAME_OVER && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                mode = INPUT_AWAIT;
                level = 1;
                orbs = 0;
                score = 0;
                chain = 0;
                lastLevelUp = 0;
                randomizeDiscs();
                restartTime = GetTime();
                continue;
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

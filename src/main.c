#include "raylib.h"
#include <locale.h>

#define SIDE 100.0
#define DISC_RADIUS SIDE / 2.0
#define RADIUS_MODIFIER 0.96
#define MAX_DISCS 7
#define MAX_DISCS_AND_NEXT_ROW 8.0
#define NEXT_DISC_ROW 8.0
#define ORB_PER_LVL 5.0
#define ORB_SIDE SIDE / 3.0
#define ORB_RADIUS ORB_SIDE / 2.0
#define LEVEL_UP_BONUS 17000
#define LEVEL_UP_INDICATOR_TIME 2.0
#define MAX_CHAIN 30
#define RESTART_TIME 1.0
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
    PRE_GAME,
    AWAITING_INPUT,
    LOWERING,
    RAISING,
    POPPING,
    BREAKING,
    GAME_OVER,
    GAME_STATE_COUNT,
} GameState;

Col cols[MAX_DISCS];
Color COLORS[MAX_DISCS + 1];
Disc nextDisc;
const int CHAIN_MULTIPLIER_MAP[MAX_CHAIN + 1] = {
    0,
    7,
    39,
    109,
    224,
    391,
    617,
    907,
    1267,
    1701,
    2213,
    2809,
    3491,
    4265,
    5133,
    6099,
    7168,
    8341,
    9622,
    11014,
    12521,
    14146,
    15891,
    17758,
    19752,
    21875,
    24128,
    26515,
    29039,
    31702,
    34506};
int mode = AWAITING_INPUT;
int level = 1;
int orbs = 0;
int score = 0;
int chain = 0;
float lastLevelUp = 0;
float restartTime = 0;
Rectangle restartButton = {
    SCREEN_WIDTH - DISC_RADIUS * 1.5,
    SCREEN_HEIGHT - DISC_RADIUS * 1.5,
    DISC_RADIUS,
    DISC_RADIUS};

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
    return nth * SIDE - DISC_RADIUS;
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
    int numberedDiscs = 0;
    for (int j = 0; j < MAX_DISCS; j++) {
        for (int i = 0; i < MAX_DISCS; i++) {
            Disc disc = cols[i].discs[j];
            if (disc.number != 0) {
                if (isBusting(disc)) {
                    breakingDiscs++;
                    cols[i].discs[j].popProgress = 0.1;
                    cols[i].discs[j].score = CHAIN_MULTIPLIER_MAP[min(chain + 1, MAX_CHAIN)];
                } else {
                    numberedDiscs++;
                }
            }
        }
    }
    if (breakingDiscs == 0) {
        if (numberedDiscs == MAX_DISCS * MAX_DISCS) {
            mode = GAME_OVER;
            return;
        }
        chain = 0;
        if (orbs == ORB_PER_LVL) {
            raiseDiscs();
            level++;
            orbs = 0;
            return;
        }
        mode = AWAITING_INPUT;
    } else {
        chain++;
        chain = min(chain, MAX_CHAIN);
        score += breakingDiscs * CHAIN_MULTIPLIER_MAP[chain];
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
    // COLUMNS
    for (int i = 0; i < MAX_DISCS; i++) {
        int iOffset = i + 1;
        float iCenter = getCenter(iOffset);
        // ROWS
        for (int j = 0; j < MAX_DISCS_AND_NEXT_ROW; j++) {
            Disc disc = cols[i].discs[j];
            if (disc.number == 0) {
                continue;
            }
            Color color = COLORS[disc.number];
            if (disc.state != BROKEN) {
                color = WHITE;
            }
            DrawCircle(iCenter, disc.currentY, DISC_RADIUS * (RADIUS_MODIFIER + disc.popProgress), color);
            if (disc.state != BROKEN) {
                DrawCircle(iCenter, disc.currentY, DISC_RADIUS * (0.8), BLACK);
                DrawCircle(iCenter, disc.currentY, DISC_RADIUS * (0.7), color);
                if (disc.state == HALF_BROKEN) {
                    DrawText(TextFormat("~", disc.number, disc.popProgress), iCenter, disc.currentY, DISC_RADIUS * 1, BLACK);
                }
            } else {
                DrawText(TextFormat("%d", disc.number), iOffset * SIDE - DISC_RADIUS, disc.currentY, DISC_RADIUS * (RADIUS_MODIFIER + disc.popProgress), WHITE);
                if (disc.score != 0) {
                    DrawText(TextFormat("+%d", disc.score), iCenter - 50, disc.currentY - 100, DISC_RADIUS, COLORS[GetRandomValue(1, MAX_DISCS)]);
                }
            }
        }
    }
}

void drawUI()
{
    float nextDiscCenter = getCenter(NEXT_DISC_ROW / 2);
    float nextDiscY = getCenter(NEXT_DISC_ROW + 1);
    float firstColX = getCenter(1);
    // UI BACKGROUND
    DrawRectangleV((Vector2){0, SCREEN_HEIGHT - 2 * SIDE}, (Vector2){SCREEN_WIDTH, SCREEN_HEIGHT}, UI_BACKGROUND);

    DrawRectangleRec(restartButton, RED);

    // draw next disc
    DrawCircle(nextDiscCenter, nextDiscY, DISC_RADIUS * RADIUS_MODIFIER, COLORS[nextDisc.number]);
    DrawText(TextFormat("%d", nextDisc.number), nextDiscCenter, nextDiscY, DISC_RADIUS * RADIUS_MODIFIER, WHITE);

    // draw orbs
    for (int i = 0; i < ORB_PER_LVL; i++) {
        float orbX = (i + 1) * ORB_SIDE - ORB_RADIUS + 20;
        DrawCircle(orbX, nextDiscY, ORB_RADIUS * 0.99, REALLYDARKBLUE);
        if (ORB_PER_LVL - i <= orbs) {
            DrawCircle(orbX, nextDiscY, ORB_RADIUS * 0.88, WHITE);
        }
    }

    // INDICATORS
    DrawText(TextFormat("Level: %d", level), ORB_RADIUS, getCenter(NEXT_DISC_ROW + 0.3), DISC_RADIUS, REALLYDARKBLUE);
    DrawText(TextFormat("Score: %'dpts", score), ORB_RADIUS, getCenter(NEXT_DISC_ROW - 0.4), DISC_RADIUS, REALLYDARKBLUE);
    if (lastLevelUp != 0 && GetTime() - LEVEL_UP_INDICATOR_TIME < lastLevelUp) {
        DrawText(TextFormat("LEVEL UP! +%d", LEVEL_UP_BONUS), getCenter((float)MAX_DISCS / 2 - 1), firstColX, DISC_RADIUS, WHITE);
    }
    if (chain > 1) {
        Color color = COLORS[GetRandomValue(1, MAX_DISCS)];
        DrawText(TextFormat("%dx", chain), getCenter(MAX_DISCS - 1), getCenter(MAX_DISCS_AND_NEXT_ROW), DISC_RADIUS * 2, color);
    }
    if (mode == GAME_OVER) {
        DrawText(TextFormat("GAME OVER"), firstColX, firstColX, DISC_RADIUS * 2, WHITE);
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

void restart()
{
    mode = PRE_GAME;
    level = 1;
    orbs = 0;
    score = 0;
    chain = 0;
    lastLevelUp = 0;
    randomizeDiscs();
    restartTime = GetTime();
}

void dropNextDisc()
{
    Vector2 mousePos = GetMousePosition();
    if (CheckCollisionPointRec(mousePos, restartButton)) {
        restart();
        mode = AWAITING_INPUT;
        restartTime = GetTime() - RESTART_TIME;
        return;
    }
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

    COLORS[0] = BACKGROUND;
    COLORS[1] = LIME;
    COLORS[2] = GOLD;
    COLORS[3] = ORANGE;
    COLORS[4] = MAROON;
    COLORS[5] = PINK;
    COLORS[6] = SKYBLUE;
    COLORS[7] = BLUE;

    randomizeDiscs();
    nextDisc = randomDisc();

    while (!WindowShouldClose()) {
        // UPDATE
        {
            if (restartTime > 0 && GetTime() - restartTime < RESTART_TIME) {
                continue;
            } else {
                restartTime = 0;
            }
            float dt = GetFrameTime();
            if (mode == GAME_OVER && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                restart();
                continue;
            }

            // if (IsKeyPressed(KEY_K)) {
            //     printf("raising\n");
            //     raiseDiscs();
            // }

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                if (mode == PRE_GAME) {
                    mode = AWAITING_INPUT;
                } else if (mode == AWAITING_INPUT) {
                    dropNextDisc();
                }
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

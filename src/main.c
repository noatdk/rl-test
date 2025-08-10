#include "raylib.h"

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
    float targetY;
    float currentY;
} Disc;

typedef struct Col {
    Disc discs[MAX_DISCS];
    Disc newDiscs[MAX_DISCS];
} Col;

typedef enum {
    INTACT,
    HALF_BROKEN,
    BROKEN,
    DISC_STATE_COUNT,
} DiscState;

typedef enum {
    INPUT_AWAIT,
    GRAVITE,
    BREAK,
    GAME_OVER,
    GAME_STATE_COUNT,
} GameState;

Col cols[MAX_DISCS];
Color colors[MAX_DISCS + 1];
int mode;

bool isBusting(Disc disc)
{
    return disc.number == disc.rowStack || disc.number == disc.colStack;
}

int randomDisc()
{
    return GetRandomValue(1, MAX_DISCS);
}

float getCenter(int nth)
{
    return nth * (float)SIDE - (float)DISC_RADIUS;
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
        for (int j = 0; j < MAX_DISCS; j++) {
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
            while (j != MAX_DISCS - 1 && cols[i].discs[nextPlacement + 1].number == 0 && cols[i].newDiscs[nextPlacement + 1].number == 0 && nextPlacement < MAX_DISCS - 1) {
                nextPlacement++;
            }
            cols[i].discs[j].currentY = getCenter(j + 1);
            cols[i].discs[j].targetY = getCenter(nextPlacement + 1);
            cols[i].newDiscs[nextPlacement] = cols[i].discs[j];
            cols[i].newDiscs[nextPlacement].currentY = cols[i].discs[j].targetY;
        }
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
            cols[i].discs[j].number = randomDisc();
            cols[i].discs[j].state = BROKEN;
            cols[i].discs[j].currentY = getCenter(j + 1);
            cols[i].discs[j].targetY = getCenter(j + 1);
            while (isBusting(cols[i].discs[j])) {
                cols[i].discs[j].number = randomDisc();
            }
        }
    }
}

void drawDiscs()
{
    // COLUMNS
    for (int i = 0; i < MAX_DISCS; i++) {
        int iOffset = i + 1;
        // ROWS
        for (int j = 0; j < MAX_DISCS; j++) {
            int jOffset = j + 1;
            Disc disc = cols[i].discs[j];
            if (disc.number == 0) {
                continue;
            }
            Color color = colors[disc.number];
            if (disc.state == INTACT) {
                color = WHITE;
            }
            DrawCircle(getCenter(iOffset), disc.currentY, (float)DISC_RADIUS, color);
            // DrawText(TextFormat("x=%d:y=%d\n%d:%d:%d", iOffset, jOffset, disc.colStack, disc.rowStack, disc.number), iOffset * SIDE - DISC_RADIUS - 15, jOffset * SIDE - DISC_RADIUS - 12, 15, BLACK);
            DrawText(TextFormat("%d", disc.number), iOffset * SIDE - DISC_RADIUS, disc.currentY, DISC_RADIUS, WHITE);
        }
    }
}

void gravitate(float dt)
{
    int movingDiscs = 0;
    for (int i = 0; i < MAX_DISCS; i++) {
        for (int j = 0; j < MAX_DISCS; j++) {
            Disc disc = cols[i].discs[j];
            if (disc.number == 0 || !(disc.targetY > disc.currentY)) {
                continue;
            }
            movingDiscs++;
            float newCurrentY = disc.currentY + dt * SIDE * 10;
            if (newCurrentY >= disc.targetY) {
                newCurrentY = disc.targetY;
            }
            cols[i].discs[j].currentY = newCurrentY;
        }
    }

    if (movingDiscs == 0) {
        swapDiscArrays(false);
        mode = INPUT_AWAIT;
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
    cols[x].discs[y].number = randomDisc();
    cols[x].discs[y].state = BROKEN;
    cols[x].discs[y].currentY = getCenter(y + 1);
    calculateDisplacements();
    mode = GRAVITE;
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "DROP7");
    SetWindowPosition(0, 0);
    // SetWindowFocused();
    SetTargetFPS(60);
    colors[0] = BACKGROUND;
    colors[1] = GREEN;
    colors[2] = YELLOW;
    colors[3] = ORANGE;
    colors[4] = RED;
    colors[5] = PINK;
    colors[6] = SKYBLUE;
    colors[7] = BLUE;
    // randomizeDiscs();

    Vector2 mousePos;

    mode = INPUT_AWAIT;

    while (!WindowShouldClose()) {
        // UPDATE
        {
            float dt = GetFrameTime();
            if (IsKeyPressed(KEY_P)) {
                randomizeDiscs();
            }
            if (mode == INPUT_AWAIT && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                dropNextDisc();
            }

            if (mode == GRAVITE) {
                gravitate(dt);
            }
        }

        // DRAWING
        BeginDrawing();
        {
            ClearBackground(BACKGROUND);
            drawDiscs();
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

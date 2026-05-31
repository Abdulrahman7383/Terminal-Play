#define _USE_MATH_DEFINES
#include <graphics.h>
#include <conio.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cmath>
#include <vector>
#include <algorithm>

// --- ≈⁄œ«œ«  «·‘«‘… Ê«·›Ì“Ì«¡ ---
#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600
#define GROUND_Y 450
#define GRAVITY 1.1
#define JUMP_FORCE -17  // ﬁÊ… «·ﬁ›“…
#define MAX_SPEED 18
#define MIN_SPEED 4
#define NORMAL_SPEED 10 // ”—⁄… «··⁄»…
#define COIN_RADIUS 10
#define MAX_COINS 3
#define MAX_OBSTACLES 2 //  ﬁ·Ì· «·⁄Ê«∆ﬁ · ‰«”» „”«— Ê«Õœ
#define DELAY_MS 20

// --- «·√·Ê«‰ ---
#define SKY_BLUE_COLOR 9
#define GRASS_GREEN_COLOR 2
#define DIRT_BROWN_COLOR 6
#define DARK_GRAY_COLOR 8
#define LIGHT_GRAY_COLOR 7
#define BIKE_RED_COLOR 4
#define BIKE_BLUE_COLOR 1
#define BIKE_YELLOW_COLOR 14
#define BIKE_LIGHTCYAN_COLOR 3

// --- Õ«·«  «··⁄»… Ê√‰Ê«⁄ «·⁄Ê«∆ﬁ ---
enum GameState { MENU, PLAYING, PAUSED, GAME_OVER };
enum ObstacleType { LOG, ROCK, LOW_BARRIER, HIGH_BARRIER };

// --- «·ÂÌ«ﬂ· (Structs) ---

struct Particle {
    float x, y, vx, vy;
    int life;
    int color;
};

struct Motorcycle {
    float x, y, vy;
    bool isGrounded;
    int width, height;

    void init() {
        x = 120;
        width = 140;
        height = 70;
        y = GROUND_Y - height;
        vy = 0;
        isGrounded = true;
    }

    void jump() {
        if (isGrounded) {
            vy = JUMP_FORCE;
            isGrounded = false;
            Beep(800, 50);
        }
    }

    void applyPhysics() {
        if (!isGrounded) {
            y += vy;
            vy += GRAVITY;
            // «· Õﬁﬁ „‰ „·«„”… «·√—÷
            if (y >= GROUND_Y - height) {
                y = GROUND_Y - height;
                vy = 0;
                isGrounded = true;
            }
        }
    }
};

struct Obstacle {
    float x;
    int type;
    bool active;

    void reset() {
        //  Ê“Ì⁄ «·⁄Ê«∆ﬁ »„”«›«  ⁄‘Ê«∆Ì… ·„‰⁄  œ«Œ·Â«
        x = SCREEN_WIDTH + 400 + rand() % 800;
        type = rand() % 4;
        active = true;
    }

    int getHeight() {
        switch(type) {
            case LOG: return 30;
            case ROCK: return 40;
            case LOW_BARRIER: return 45;
            case HIGH_BARRIER: return 65;
            default: return 35;
        }
    }
};

struct Coin {
    float x, y;
    bool active;

    void reset() {
        x = SCREEN_WIDTH + rand() % 800;
        // «·⁄„·«  ≈„« √‰  ﬂÊ‰ ﬁ—Ì»… „‰ «·√—÷ √Ê „— ›⁄… ·  ÿ·» ﬁ›“…
        if (rand() % 2 == 0)
            y = GROUND_Y - 40;
        else
            y = GROUND_Y - 120;

        active = true;
    }
};

// --- «·„ €Ì—«  «·⁄«„… ---
Motorcycle player;
std::vector<Obstacle> obstacles(MAX_OBSTACLES);
std::vector<Coin> coins(MAX_COINS);
std::vector<Particle> particles;

GameState gameState = MENU;
int score = 0, highScore = 0, level = 1;
int currentSpeed = NORMAL_SPEED;
int lives = 3;
int invincibleTimer = 0;
int page = 0; // Double buffering

long distanceTraveled = 0;
long nextLevelDistance = 2000;

// --- œÊ«· «·—”„ ---

void drawBackgroundAndGround() {
    // «·”„«¡
    setfillstyle(SOLID_FILL, SKY_BLUE_COLOR);
    bar(0, 0, SCREEN_WIDTH, GROUND_Y);

    // «·⁄‘» «·√—÷Ì
    setfillstyle(SOLID_FILL, GRASS_GREEN_COLOR);
    bar(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y + 20);

    // «· —»… («· —«»)
    setfillstyle(SOLID_FILL, DIRT_BROWN_COLOR);
    bar(0, GROUND_Y + 20, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Œÿ «·√—÷Ì…
    setcolor(WHITE);
    setlinestyle(SOLID_LINE, 0, 3);
    line(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y);
    setlinestyle(SOLID_LINE, 0, 1); // ≈⁄«œ… «·Œÿ ··ÕÃ„ «·ÿ»Ì⁄Ì
}

void drawMotorcycle(float x, float y) {
    if (invincibleTimer > 0 && (invincibleTimer / 3) % 2 == 0) return;

    // ÂÌﬂ· «·œ—«Ã… «·√”«”Ì
    setfillstyle(SOLID_FILL, BIKE_RED_COLOR);
    bar(x + 30, y + 30, x + 100, y + 50);

    // «·⁄Ã·« 
    setfillstyle(SOLID_FILL, BLACK);
    fillellipse(x + 40, y + 55, 15, 15);
    fillellipse(x + 90, y + 55, 15, 15);

    // «·”«∆ﬁ (»‘ﬂ· „»”ÿ)
    setfillstyle(SOLID_FILL, BIKE_BLUE_COLOR);
    fillellipse(x + 65, y + 15, 10, 10); // «·—√”
    bar(x + 60, y + 25, x + 70, y + 40); // «·Ã”„
}

void drawObstacles() {
    for (auto& obs : obstacles) {
        if (!obs.active) continue;
        setfillstyle(SOLID_FILL, (obs.type >= 2) ? RED : DARKGRAY);
        // —”„ «·⁄«∆ﬁ „·«„”« ··√—÷
        bar(obs.x, GROUND_Y - obs.getHeight(), obs.x + 40, GROUND_Y);
    }
}

void drawCoins() {
    for (auto& coin : coins) {
        if (!coin.active) continue;
        setfillstyle(SOLID_FILL, YELLOW);
        fillellipse(coin.x, coin.y, COIN_RADIUS, COIN_RADIUS);
    }
}

void drawUI() {
    char text[100];
    setcolor(WHITE);
    setbkcolor(SKY_BLUE_COLOR); // ·ÌﬂÊ‰ Œ·›Ì… «·‰’ ‰›” ·Ê‰ «·”„«¡
    sprintf(text, "SCORE: %d | LEVEL: %d | LIVES: %d", score, level, lives);
    outtextxy(20, 20, text);
}

// ---  ÕœÌÀ „‰ÿﬁ «··⁄»… ---

void updateGame() {
    if (gameState != PLAYING) return;

    if (invincibleTimer > 0) invincibleTimer--;
    player.applyPhysics();
    distanceTraveled += currentSpeed;

    //  ÕœÌÀ «·⁄Ê«∆ﬁ Ê«· ’«œ„
    for (auto& obs : obstacles) {
        obs.x -= currentSpeed;
        if (obs.x < -50) obs.reset();

        // ›Õ’ «· ’«œ„
        if (invincibleTimer == 0 && obs.active) {
            //  œ«Œ· ›Ì «·„ÕÊ— X
            if (obs.x < player.x + 90 && obs.x + 40 > player.x + 30) {
                //  œ«Œ· ›Ì «·„ÕÊ— Y ( Õﬁﬁ „« ≈–« ﬂ«‰  «·œ—«Ã… ·„  ﬁ›“ √⁄·Ï „‰ «·⁄«∆ﬁ)
                if (player.y + player.height > GROUND_Y - obs.getHeight()) {
                    lives--;
                    invincibleTimer = 50;
                    Beep(400, 100);
                    if (lives <= 0) gameState = GAME_OVER;
                }
            }
        }
    }

    //  ÕœÌÀ «·⁄„·« 
    for (auto& coin : coins) {
        coin.x -= currentSpeed;
        if (coin.x < -20) coin.reset();

        if (coin.active && abs(coin.x - (player.x + 60)) < 40) {
            //  œ«Œ· «·⁄„·… „⁄ «·œ—«Ã… ⁄„ÊœÌ«
            if (coin.y > player.y && coin.y < player.y + player.height + 20) {
                score += 10;
                coin.active = false;
                Beep(1200, 50);
            }
        }
    }
}

void handleInput() {
    if (kbhit()) {
        int key = getch();
        if (key == 224) { // „›« ÌÕ «·√”Â„
            key = getch();
            if (key == 72) player.jump(); // ”Â„ ··√⁄·Ï («·ﬁ›“)
        } else if (key == ' ') {
            player.jump(); // „”ÿ—… «·„”«›«  («·ﬁ›“)
        } else if (key == 13 && (gameState == MENU || gameState == GAME_OVER)) {
            // ≈⁄«œ…  ⁄ÌÌ‰ «··⁄»… »÷€ÿ Enter
            player.init();
            score = 0; lives = 3; distanceTraveled = 0;
            for (auto& obs : obstacles) obs.reset();
            // Ê÷⁄ „”«›… ≈÷«›Ì… ··⁄«∆ﬁ «·√Ê· · ›«œÌ «·„Ê  «·›Ê—Ì
            obstacles[0].x += 500;
            for (auto& coin : coins) coin.reset();
            gameState = PLAYING;
        }
    }
}

int main() {
    //  ÂÌ∆… «·Ã—«›Ìﬂ”
    int gd = DETECT, gm;
    initgraph(&gd, &gm, "");

    // «·ﬁÌ„ «·„»œ∆Ì…
    player.init();
    for (auto& obs : obstacles) obs.reset();
    for (auto& coin : coins) coin.reset();

    // Õ·ﬁ… «··⁄»… «·√”«”Ì…
    while (true) {
        setactivepage(page);
        cleardevice();

        handleInput();

        if (gameState == MENU) {
            setbkcolor(BLACK);
            setcolor(WHITE);
            outtextxy(SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2, "PRESS ENTER TO START");
        } else if (gameState == PLAYING) {
            updateGame();
            drawBackgroundAndGround();
            drawObstacles();
            drawCoins();
            drawMotorcycle(player.x, player.y);
            drawUI();
        } else if (gameState == GAME_OVER) {
            drawBackgroundAndGround();
            drawObstacles();
            drawMotorcycle(player.x, player.y);
            setbkcolor(BLACK);
            setcolor(RED);
            outtextxy(SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 - 20, "GAME OVER!");
            setcolor(WHITE);
            outtextxy(SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 + 10, "PRESS ENTER TO RESTART");
        }

        setvisualpage(page);
        page = 1 - page; // «· »œÌ· »Ì‰ 0 Ê 1 (Double Buffering)
        delay(DELAY_MS);

        // «·Œ—ÊÃ »÷€ÿ… “— Esc
        if (GetAsyncKeyState(VK_ESCAPE)) break;
    }

    closegraph();
    return 0;
}

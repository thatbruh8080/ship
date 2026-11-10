#include <graphx.h>
#include <keypadc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fileioc.h>
#include "gfx/gfx.h"

#define SAVE_FILE "SHIPSAV"
enum { LEFT_EDGE = 127, RIGHT_EDGE = 304, EXTRA_LIFE = 5000 };
enum { PATROL, DIVE };

/* Zero means no explosion. Ticks 1 through 18 select the three sprites. */
static void drawExplosion(unsigned char tick, int x, int y)
{
    gfx_sprite_t *sprite = boom1;
    if (tick > 12)
        sprite = boom3;
    else if (tick > 6)
        sprite = boom2;
    gfx_TransparentSprite(sprite, x, y);
}

static void updateDigits(int value, unsigned char digits[6])
{
    for (int i = 5; i >= 0; --i) {
        digits[i] = value % 10;
        value /= 10;
    }
}

static void drawDigits(const unsigned char digits[6], int y)
{
    for (int i = 0; i < 6; ++i)
        gfx_Sprite(scorenum_tiles[digits[i]], 10 + i * 6, y);
}

/* Both ships have a 16 x 14 hitbox. The enemy's starts one pixel lower. */
static bool hitsShip(int x, int y, int width, int height, int shipX, int shipY)
{
    return x < shipX + 16 && x + width > shipX &&
           y < shipY + 14 && y + height > shipY;
}

static int loadHighScore(void)
{
    int value = 10000;
    ti_var_t file = ti_Open(SAVE_FILE, "r");
    if (file) {
        if (ti_Read(&value, sizeof(value), 1, file) != 1 || value < 0)
            value = 10000;
        ti_Close(file);
    }
    return value;
}

static void saveHighScore(int value)
{
    ti_var_t file = ti_Open(SAVE_FILE, "w");
    if (file) {
        if (ti_Write(&value, sizeof(value), 1, file) == 1)
            ti_SetArchiveStatus(true, file);
        ti_Close(file);
    }
}

static int spawnX(void)
{
    return LEFT_EDGE + rand() % (RIGHT_EDGE - LEFT_EDGE);
}

int main(void)
{
    int x = 218;
    int y = 160;
    int bgScroll = 0;
    unsigned char frame = 0;

    bool bulletActive = false;
    int bulletX = 0;
    int bulletY = 0;

    bool enemyBulletActive = false;
    int enemyBulletX = 0;
    int enemyBulletY = 0;

    int enemyX = spawnX();
    int enemyY = -20;
    int enemyVX = 1;
    int enemyDir = 1;

    int multiplier = 1;
    int comboStreak = 0;
    int comboTimer = 0;

    int enemyState = DIVE;

    unsigned char enemyExplosion = 0;

    bool playerRespawning = false;
    int invincibleTimer = 0;

    int enemyDiveDelay = 0;
    int enemyShootingTime = 30;

    bool playerAlive = true;
    unsigned char playerExplosion = 0;
    int lives = 3;

    int scoreValue = 0;
    const int savedHigh = loadHighScore();
    int highScoreValue = savedHigh;
    int nextLifeScore = EXTRA_LIFE;
    int displayedScore = 0;
    unsigned char scoreDigits[6] = {0};
    unsigned char highDigits[6];
    updateDigits(highScoreValue, highDigits);

    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_SetTransparentColor(0);
    gfx_SetDrawBuffer();

    while (true) {
        kb_Scan();
        if (kb_IsDown(kb_KeyClear))
            break;
        ++frame;

        if (!playerAlive && kb_IsDown(kb_Key2nd) && !playerExplosion) {
            if (lives == 0) {
                scoreValue = 0;
                lives = 3;
                nextLifeScore = EXTRA_LIFE;
            }

            invincibleTimer = 180;
            playerRespawning = true;
            playerAlive = true;

            x = 218;
            y = GFX_LCD_HEIGHT + 10;
        }

        if (invincibleTimer > 0)
            --invincibleTimer;

        /* Combo decay */
        if (comboTimer > 0) {
            comboTimer--;
        } else {
            comboStreak = 0;
            multiplier = 1;
        }

        /* Player movement */

        if (playerAlive) {
            if (kb_IsDown(kb_KeyLeft))  x -= 2;
            if (kb_IsDown(kb_KeyRight)) x += 2;
        }

        if (x < LEFT_EDGE)
            x = LEFT_EDGE;

        if (x > RIGHT_EDGE - ship1_width)
            x = RIGHT_EDGE - ship1_width;

        /* Fly back in from below the screen. */
        if (playerRespawning) {
            y -= 4;

            if (y <= 160) {
                y = 160;
                playerRespawning = false;
            }
        }

        /* Player shooting */

        if (playerAlive && kb_IsDown(kb_Key2nd) && !bulletActive) {
            bulletActive = true;
            bulletX = x + ship1_width / 2 - 1;
            bulletY = y - 4;
        }

        if (bulletActive) {
            bulletY -= 4;

            if (bulletY < 0) {
                bulletActive = false;

                comboStreak = 0;
                multiplier = 1;
            }
        }

        /* Enemy movement */

        if (!enemyExplosion) {
            if (enemyState == PATROL) {
                enemyX += enemyDir;

                if (enemyX <= LEFT_EDGE) {
                    enemyX = LEFT_EDGE;
                    enemyDir = 1;
                }

                if (enemyX >= RIGHT_EDGE - 16) {
                    enemyX = RIGHT_EDGE - 16;
                    enemyDir = -1;
                }

                if (playerAlive && enemyX < x + 16 && enemyX + 16 > x && enemyDiveDelay == 0) {
                    enemyDiveDelay = 15;
                }

                if (enemyDiveDelay > 0) {
                    enemyDiveDelay--;

                    if (enemyDiveDelay == 0)
                        enemyState = DIVE;
                }
            } else {
                enemyY += 3;

                /* Chase a vulnerable ship. */
                if (playerAlive && !invincibleTimer) {
                    if (x > enemyX + 2)
                        enemyVX = 1;
                    else if (x < enemyX - 2)
                        enemyVX = -1;
                    else
                        enemyVX = 0;
                }

                /* Flee a respawning ship. */
                else if (invincibleTimer) {
                    if (x < enemyX)
                        enemyVX = 2;   // move right (away)
                    else if (x > enemyX)
                        enemyVX = -2;  // move left (away)
                }

                enemyX += enemyVX;

                if (enemyX <= LEFT_EDGE) {
                    enemyX = LEFT_EDGE;
                    enemyVX = -enemyVX;
                }

                if (enemyX >= RIGHT_EDGE - 16) {
                    enemyX = RIGHT_EDGE - 16;
                    enemyVX = -enemyVX;
                }

                if (enemyY > GFX_LCD_HEIGHT)
                    enemyY = -20;

                if (enemyY >= 35 && enemyY < 38) {
                    enemyY = 35;
                    enemyState = PATROL;
                    enemyDiveDelay = 0;
                }
            }
        }

        /* Enemy shooting */

        if (!enemyExplosion && !enemyBulletActive && playerAlive && enemyShootingTime == 0 && !invincibleTimer) {
            enemyBulletActive = true;
            enemyBulletX = enemyX + 8;
            enemyBulletY = enemyY + 20;
        }

        if (enemyShootingTime > 0)
            enemyShootingTime--;

        if (enemyBulletActive) {
            enemyBulletY += 4;

            if (enemyBulletY > GFX_LCD_HEIGHT) {
                enemyBulletActive = false;
                enemyShootingTime = 30;
            }
        }

        /* Player bullet hits enemy */

        if (bulletActive && !enemyExplosion &&
            hitsShip(bulletX, bulletY, 2, 10, enemyX, enemyY + 1)) {
            bulletActive = false;
            enemyExplosion = 1;
            /* At 147 hits the multiplier reaches its cap of 50. */
            if (comboStreak < 147)
                ++comboStreak;
            comboTimer = 180;
            multiplier = 1 + comboStreak / 3;
            scoreValue += 100 * multiplier;
        }

        if (enemyExplosion && ++enemyExplosion > 18) {
            enemyExplosion = 0;
            enemyX = spawnX();
            enemyY = -20;
            enemyState = DIVE;
            enemyDiveDelay = 0;
        }
        if (playerExplosion && ++playerExplosion > 18)
            playerExplosion = 0;

        bool playerHit = false;
        if (enemyBulletActive && playerAlive && !invincibleTimer &&
            hitsShip(enemyBulletX, enemyBulletY, 2, 10, x, y)) {
            enemyBulletActive = false;
            playerHit = true;
        }

        if (!playerHit && !enemyExplosion && playerAlive &&
            hitsShip(enemyX, enemyY + 1, 16, 14, x, y)) {
            enemyExplosion = 1;
            if (invincibleTimer)
                scoreValue += 100 * multiplier;
            else
                playerHit = true;
        }

        if (playerHit) {
            --lives;
            playerAlive = false;
            playerExplosion = 1;
            comboStreak = 0;
            comboTimer = 0;
            multiplier = 1;
        }

        /* Only split the score into decimal digits when it changes. */
        if (scoreValue != displayedScore) {
            displayedScore = scoreValue;
            updateDigits(scoreValue, scoreDigits);
            if (scoreValue > highScoreValue) {
                highScoreValue = scoreValue;
                for (int i = 0; i < 6; ++i)
                    highDigits[i] = scoreDigits[i];
            }
            if (scoreValue >= nextLifeScore) {
                ++lives;
                nextLifeScore += EXTRA_LIFE;
            }
        }

        /* Background */

        --bgScroll;

        if (bgScroll <= -bg_height)
            bgScroll = 0;

        /* The opaque scrolling tiles already cover the entire right strip. */
        int bgX = GFX_LCD_WIDTH - bg_width;
        gfx_SetColor(0);
        gfx_FillRectangle(0, 0, bgX, GFX_LCD_HEIGHT);
        for (int top = -bgScroll - bg_height; top < GFX_LCD_HEIGHT; top += bg_height) {
            if (top > -bg_height)
                gfx_Sprite(bg, bgX, top);
        }

        if (enemyExplosion)
            drawExplosion(enemyExplosion, enemyX, enemyY);
        else
            gfx_TransparentSprite(enemy, enemyX, enemyY);

        if (playerExplosion)
            drawExplosion(playerExplosion, x, y);
        else if (playerAlive && (!invincibleTimer || (frame & 4)))
            gfx_TransparentSprite((frame & 4) ? ship1 : ship2, x, y);

        /* Bullets */

        gfx_SetColor(255);
        if (bulletActive) {
            gfx_FillRectangle(bulletX, bulletY, 2, 10);
        }

        if (enemyBulletActive) {
            gfx_FillRectangle(enemyBulletX, enemyBulletY, 2, 10);
        }

        if (!playerAlive && !playerExplosion && lives == 0) {
            gfx_Sprite(gameova, 150, 125);

            if (scoreValue == highScoreValue)
            {
                gfx_Sprite(newhiscore, 9, 110);
            }
        }

        /* HUD labels */

        gfx_Sprite(score, 10, 19);
        gfx_Sprite(hiscore, 10, 73);
        gfx_Sprite(lifecounter, 10, 184);
        /* Generated tilesets may expose their count. Otherwise only the
           four indices used by the starting lives are known to exist. */
#ifdef lifenum_num_tiles
        const int lifeTileCount = lifenum_num_tiles;
#else
        const int lifeTileCount = 4;
#endif
        if (lives < lifeTileCount)
            gfx_Sprite(lifenum_tiles[lives], 37, 188);
        else {
            unsigned char digits[6];
            updateDigits(lives, digits);
            int first = 0;
            while (first < 5 && digits[first] == 0)
                ++first;
            for (int i = first; i < 6; ++i)
                gfx_Sprite(scorenum_tiles[digits[i]], 37 + 6 * (i - first), 188);
        }
        drawDigits(scoreDigits, 35);
        drawDigits(highDigits, 89);
        gfx_SwapDraw();
    }

    if (highScoreValue > savedHigh)
        saveHighScore(highScoreValue);
    gfx_End();
    return 0;
}

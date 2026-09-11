#include <graphx.h>
#include <keypadc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fileioc.h>
#include "gfx/gfx.h"

#define SAVE_FILE "SHIPSAV"
void saveHighScore(int highScore);

/* convert score to digits */
void updateDigits(int value, int digits[6])
{
    for (int i = 5; i >= 0; i--) {
        digits[i] = value % 10;
        value /= 10;
    }
}

void loadHighScore(int *highScore)
{
    ti_var_t file = ti_Open(SAVE_FILE, "r");

    if (file) {
        ti_Read(highScore, sizeof(int), 1, file);
        ti_Close(file);
    } else {
        *highScore = 10000;
    }
}

void saveHighScore(int highScore)
{
    ti_Delete(SAVE_FILE);

    ti_var_t file = ti_Open(SAVE_FILE, "w");

    if (file) {
        ti_Write(&highScore, sizeof(int), 1, file);
        ti_SetArchiveStatus(true, file);
        ti_Close(file);
    }
}

int main(void)
{
    int x = 218;
    int y = 160;
    int bgScroll = 0;
    int frame = 0;

    bool bulletActive = false;
    int bulletX = 0;
    int bulletY = 0;

    bool enemyBulletActive = false;
    int enemyBulletX = 0;
    int enemyBulletY = 0;

    int enemyX = 207;
    int enemyY = 35;
    int enemyVX = 1; 
    int enemyDir = 1;

    int multiplier = 1;
    int comboStreak = 0;
    int comboTimer = 0;

    int enemyState = 0;

    bool enemyAlive = true;
    bool enemyExploding = false;

    bool playerRespawning = false;
    bool playerInvincible = false;

    int invincibleTimer = 0;

    int explosionFrame = 0;
    int explosionTimer = 0;

    int enemyDiveDelay = 0;
    int enemyShootingTime = 30;

    bool playerAlive = true;
    bool playerExploding = false;
    int lives = 3;

    int playerExplosionFrame = 0;
    int playerExplosionTimer = 0;

    /* SCORE SYSTEM */

    int scoreValue = 0;
    int highScoreValue = 10000;
    int nextLifeScore = 5000;

    loadHighScore(&highScoreValue);

    int scoreDigits[6] = {0,0,0,0,0,0};

    int highDigits[6] = {0,0,0,0,0,0};

    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_SetTransparentColor(0);
    gfx_SetDrawBuffer();

    enemyX = 127 + rand() % (304 - 127);
    enemyY = -20;
    enemyState = 1;

    while (!kb_IsDown(kb_KeyClear))
    {
        kb_Scan();
        frame++;

        updateDigits(scoreValue, scoreDigits);
        updateDigits(highScoreValue, highDigits);

        if (!playerAlive && kb_IsDown(kb_Key2nd) && !playerRespawning && !playerExploding) {

            if(lives == 0){
                scoreValue = 0;
                lives = 3;
            }

            invincibleTimer = 180;
            playerInvincible = true;
            playerRespawning = true;
            playerAlive = true;

            x = 218;          /* center */
            y = GFX_LCD_HEIGHT + 10;   /* start below screen */

        }

        /*Timer*/
        if (playerInvincible) {

            invincibleTimer--;

            if (invincibleTimer <= 0)
                playerInvincible = false;
        }

        /*Combo decay*/
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

        if (x < 127)
            x = 127;

        if (x > 304 - ship1_width)
            x = 304 - ship1_width;

        /*Flying up*/
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

            if (bulletY < 0){
                bulletActive = false;

                comboStreak = 0;
                multiplier = 1;
            }
        }

        /* Enemy movement */

        if (enemyAlive) {

            if (enemyState == 0) {

                enemyX += enemyDir;

                if (enemyX <= 127) {
                    enemyX = 127;
                    enemyDir = 1;
                }

                if (enemyX >= 304 - 16) {
                    enemyX = 304 - 16;
                    enemyDir = -1;
                }

                if (playerAlive && enemyX < x + 16 && enemyX + 16 > x && enemyDiveDelay == 0) {
                    enemyDiveDelay = 15;
                }

                if (enemyDiveDelay > 0) {
                    enemyDiveDelay--;

                    if (enemyDiveDelay == 0)
                        enemyState = 1;
                }
            }

            else if (enemyState == 1) {

                enemyY += 3;

                /* NORMAL behavior (chase player) */
                if (playerAlive && !playerInvincible) {

                    if (x > enemyX + 2)
                        enemyVX = 1;
                    else if (x < enemyX - 2)
                        enemyVX = -1;
                    else
                        enemyVX = 0;
                }

                /* SCARED behavior (run away + bounce) */
                else if (playerInvincible) {

                    if (x < enemyX)
                        enemyVX = 2;   // move right (away)
                    else if (x > enemyX)
                        enemyVX = -2;  // move left (away)
                }

                /* APPLY movement */
                enemyX += enemyVX;

                /* BOUNCE OFF WALLS */
                if (enemyX <= 127) {
                    enemyX = 127;
                    enemyVX = -enemyVX;
                }

                if (enemyX >= 304 - 16) {
                    enemyX = 304 - 16;
                    enemyVX = -enemyVX;
                }

                if (enemyY > GFX_LCD_HEIGHT)
                    enemyY = -20;

                if (enemyY >= 35 && enemyY < 38) {
                    enemyY = 35;
                    enemyState = 0;
                    enemyDiveDelay = 0;
                }
            }
        }

        /* Enemy shooting */

        if (enemyAlive && !enemyBulletActive && playerAlive && enemyShootingTime == 0 && !playerInvincible) {
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

        if (bulletActive && enemyAlive) {

            int enemyHitX = enemyX;
            int enemyHitY = enemyY + 1;

            if (bulletX < enemyHitX + 16 &&
                bulletX + 2 > enemyHitX &&
                bulletY < enemyHitY + 14 &&
                bulletY + 10 > enemyHitY)
            {
                bulletActive = false;

                enemyAlive = false;
                enemyExploding = true;

                explosionFrame = 0;
                explosionTimer = 0;

                /* TEST SCORE */


                comboStreak++;
                comboTimer = 180;   // ~2 seconds at 60 FPS

                /* smooth growth curve */
                multiplier = 1 + comboStreak / 3;

                /* optional soft cap (feels better than infinite explosion) */
                if (multiplier > 50)
                    multiplier = 50;

                scoreValue += 100 * multiplier;

                if (scoreValue >= nextLifeScore) {
                    lives++;
                    nextLifeScore += 5000;
                }

                if (scoreValue > highScoreValue) {
                    highScoreValue = scoreValue;
                }
            }
        }

        /* Enemy explosion */

        if (enemyExploding) {

            explosionTimer++;

            if (explosionTimer > 5) {

                explosionTimer = 0;
                explosionFrame++;

                if (explosionFrame >= 3) {

                    enemyExploding = false;

                    enemyAlive = true;

                    /* random X between 127 and 304 */
                    enemyX = 127 + rand() % (304 - 127);
                    enemyY = -20;     /* start above screen */
                    enemyState = 1;
                    enemyDiveDelay = 0;

                }
            }
        }

        /* Player explosion */

        if (playerExploding) {

            playerExplosionTimer++;

            if (playerExplosionTimer > 5) {

                playerExplosionTimer = 0;
                playerExplosionFrame++;

                if (playerExplosionFrame >= 3)
                    playerExploding = false;
            }
        }

        /* Enemy bullet hits player */

        if (enemyBulletActive && playerAlive && !playerInvincible) {

            int playerHitX = x;
            int playerHitY = y;

            if (enemyBulletX < playerHitX + 16 &&
                enemyBulletX + 2 > playerHitX &&
                enemyBulletY < playerHitY + 14 &&
                enemyBulletY + 10 > playerHitY)
            {
                enemyBulletActive = false;

                comboStreak = 0;
                multiplier = 1;
                comboTimer = 0;

                playerAlive = false;
                playerExploding = true;

                playerExplosionFrame = 0;
                playerExplosionTimer = 0;
                lives--;
            }
        }

        /* Player / enemy collision */

        if (enemyAlive && playerAlive && !playerExploding) {

            int enemyHitX = enemyX;
            int enemyHitY = enemyY + 1;

            int playerHitX = x;
            int playerHitY = y;

            if (enemyHitX < playerHitX + 16 &&
                enemyHitX + 16 > playerHitX &&
                enemyHitY < playerHitY + 14 &&
                enemyHitY + 14 > playerHitY)
            {
                if (playerInvincible) {
                    enemyAlive = false;
                    enemyExploding = true;

                    explosionFrame = 0;
                    explosionTimer = 0;

                    scoreValue += 100 * multiplier;

                } else {

                    playerAlive = false;
                    playerExploding = true;

                    playerExplosionFrame = 0;
                    playerExplosionTimer = 0;

                    enemyAlive = false;
                    enemyExploding = true;

                    explosionFrame = 0;
                    explosionTimer = 0;

                    if (lives > 0){
                        lives--;
                    }

                    /* reset combo */
                    comboStreak = 0;
                    multiplier = 1;
                }
            }
        }

        /* Background */

        bgScroll --;

        if (bgScroll <= -bg_height)
            bgScroll = 0;

        gfx_FillScreen(0);

        int bgX = GFX_LCD_WIDTH - bg_width;

        for (int i = -bg_height; i < GFX_LCD_HEIGHT + bg_height; i += bg_height)
            gfx_Sprite(bg, bgX, i - bgScroll);

        /* Enemy */

        if (enemyAlive)
            gfx_TransparentSprite(enemy, enemyX, enemyY);

        if (enemyExploding) {

            if (explosionFrame == 0)
                gfx_TransparentSprite(boom1, enemyX, enemyY);

            if (explosionFrame == 1)
                gfx_TransparentSprite(boom2, enemyX, enemyY);

            if (explosionFrame == 2)
                gfx_TransparentSprite(boom3, enemyX, enemyY);
        }

        /* Player */

        if (playerAlive){

            if (!playerInvincible || (frame & 4)) {

                if (frame & 4)
                    gfx_TransparentSprite(ship1, x, y);
                else
                    gfx_TransparentSprite(ship2, x, y);
            }
        }

        if (playerExploding) {

            if (playerExplosionFrame == 0)
                gfx_TransparentSprite(boom1, x, y);

            if (playerExplosionFrame == 1)
                gfx_TransparentSprite(boom2, x, y);

            if (playerExplosionFrame == 2)
                gfx_TransparentSprite(boom3, x, y);
        }

        /* Bullets */

        if (bulletActive) {
            gfx_SetColor(255);
            gfx_FillRectangle(bulletX, bulletY, 2, 10);
        }

        if (enemyBulletActive) {
            gfx_SetColor(255);
            gfx_FillRectangle(enemyBulletX, enemyBulletY, 2, 10);
        }

        if (!playerAlive && !playerExploding && lives == 0){

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
        gfx_Sprite(lifenum_tiles[lives], 37,188);

        /* SCORE digits */

        for (int i = 0; i < 6; i++) {
            gfx_Sprite(scorenum_tiles[scoreDigits[i]], 10 + i * 6, 35);
        }

        /* HIGH SCORE digits */

        for (int i = 0; i < 6; i++) {
            gfx_Sprite(scorenum_tiles[highDigits[i]], 10 + i * 6, 89);
        }

        gfx_SwapDraw();
    }

    saveHighScore(highScoreValue);
    gfx_End();
    return 0;
}
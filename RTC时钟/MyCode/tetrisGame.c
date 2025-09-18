#include "main.h"
#include "lcd.h"
#include "tetrisGame.h"
#include "string.h"

// 记录上次按下时间
static uint32_t lastTick_PA0 = 0;
static uint32_t lastTick_PE2 = 0;
static uint32_t lastTick_PE3 = 0;
static uint32_t lastTick_PE4 = 0;

// ================= 配置 =================
#define MAP_W 10
#define MAP_H 20
#define BLOCK_SIZE 12  // 每个方格大小（像素）

extern KeyState key_state;   // 引用 key.c 里定义的全局变量
int score = 0;   // 玩家得分


// 方块形状定义（7种）
const uint8_t tetromino[7][4][4] = {
    { // I
        {0,0,0,0},
        {1,1,1,1},
        {0,0,0,0},
        {0,0,0,0}
    },
    { // O
        {1,1,0,0},
        {1,1,0,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    { // T
        {0,1,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    { // L
        {0,0,1,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    { // J
        {1,0,0,0},
        {1,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    { // S
        {0,1,1,0},
        {1,1,0,0},
        {0,0,0,0},
        {0,0,0,0}
    },
    { // Z
        {1,1,0,0},
        {0,1,1,0},
        {0,0,0,0},
        {0,0,0,0}
    }
};

// ================= 游戏数据结构 =================
uint8_t map[MAP_H][MAP_W] = {0}; // 游戏区域

Block current;

// ================= 工具函数 =================

// 生成一个新的方块
void newBlock(void) {
    current.type = rand() % 7;
    current.x = MAP_W / 2 - 2;
    current.y = 0;
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            current.shape[i][j] = tetromino[current.type][i][j];
}

// 碰撞检测
int checkCollision(int nx, int ny, uint8_t shape[4][4]) {
    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            if(shape[i][j]){
                int x = nx + j;
                int y = ny + i;
                if(x<0 || x>=MAP_W || y>=MAP_H) return 1;
                if(y>=0 && map[y][x]) return 1;
            }
        }
    }
    return 0;
}

// 固定方块到地图
void lockBlock(void) {
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            if(current.shape[i][j]){
                int x = current.x + j;
                int y = current.y + i;
                if(y>=0) map[y][x] = 1;
            }
}

// 消除满行
void clearLines(void) {
    for(int i = MAP_H - 1; i >= 0; i--) {
        int full = 1;
        for(int j = 0; j < MAP_W; j++) {
            if(!map[i][j]) full = 0;
        }
        if(full) {
            // 消行
            for(int k = i; k > 0; k--) {
                for(int j = 0; j < MAP_W; j++) {
                    map[k][j] = map[k - 1][j];
                }
            }
            for(int j = 0; j < MAP_W; j++) map[0][j] = 0;

            score++;   // <<< 加分
            i++;       // 重新检查该行
        }
    }
}


// 绘制界面
void drawGame(void) {
    LCD_Clear(WHITE);

    // 绘制固定方块
    for(int i = 0; i < MAP_H; i++) {
        for(int j = 0; j < MAP_W; j++) {
            if(map[i][j]) {
                LCD_Fill(j * BLOCK_SIZE, i * BLOCK_SIZE, 
                         j * BLOCK_SIZE + BLOCK_SIZE - 1, 
                         i * BLOCK_SIZE + BLOCK_SIZE - 1, YELLOW);
            }
        }
    }

    // 绘制当前方块
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            if(current.shape[i][j]) {
                int x = current.x + j;
                int y = current.y + i;
                if(y >= 0) {
                    LCD_Fill(x * BLOCK_SIZE, y * BLOCK_SIZE, 
                             x * BLOCK_SIZE + BLOCK_SIZE - 1, 
                             y * BLOCK_SIZE + BLOCK_SIZE - 1, GREEN);
                }
            }
        }
    }

   // 显示分数
   LCD_ShowString(130, 10, 200, 16, 24, (uint8_t*)"Score:");  
   LCD_ShowNum(190, 10, score, 3, 24);   // 在 (190,10) 显示分数，宽度3位，16号字体

}


// 顺时针旋转 90 度
void rotateCW(uint8_t shape[4][4]) {
    uint8_t tmp[4][4];
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            tmp[j][3 - i] = shape[i][j];
        }
    }
    memcpy(shape, tmp, sizeof(tmp));
}


void tetrisGame(void) {
    int exitFlag = 0;
    newBlock();

    uint32_t lastTick = HAL_GetTick();
    uint32_t fallInterval = 300; // 自动下落间隔 ms

    drawGame();  // 初始绘制

    while(!exitFlag) {
        Key_Scan();   // 每个循环调用轮询函数，检测并去抖

        // ---------- 按键处理 ----------
        switch(key_state) {
            case KEY_LEFT:
                if(!checkCollision(current.x - 1, current.y, current.shape)) {
                    current.x--;
                    drawGame();
                }
                key_state = KEY_DISABLE;
                break;

            case KEY_RIGHT:
                if(!checkCollision(current.x + 1, current.y, current.shape)) {
                    current.x++;
                    drawGame();
                }
                key_state = KEY_DISABLE;
                break;

            case KEY_DOWM:   // 瞬间落到底
                while(!checkCollision(current.x, current.y + 1, current.shape)) {
                    current.y++;
                }
                drawGame();
                key_state = KEY_DISABLE;
                break;

            case KEY_UP:  // KEY_UP -> 顺时针旋转方块
               {
                   uint8_t tmp[4][4];
                   memcpy(tmp, current.shape, sizeof(tmp));   // 拷贝当前形状
                   rotateCW(tmp);                             // 顺时针旋转
                   if(!checkCollision(current.x, current.y, tmp)) {
                       memcpy(current.shape, tmp, sizeof(tmp));   // 更新方块
                       drawGame();                               // 立即刷新界面
                   }
                   key_state = KEY_DISABLE;  // 处理完清零
               }
            break;


            default:
                break;
        }

        // ---------- 自动下落 ----------
        uint32_t now = HAL_GetTick();
        if(now - lastTick >= fallInterval) {
            if(!checkCollision(current.x, current.y + 1, current.shape)) {
                current.y++;
            } else {
                lockBlock();
                clearLines();
                newBlock();

                if(checkCollision(current.x, current.y, current.shape)) {
                    LCD_ShowString(30, 100, 200, 16, 16, (uint8_t*)"Game Over!");
                    exitFlag = 1;
                }
            }
            lastTick = now;
            drawGame();
        }

        HAL_Delay(20); // 限制刷新频率，防止闪烁
    }
}


// 轮询检测函数
void Key_Scan(void)
{
    uint32_t now = HAL_GetTick();

    // KEY_UP (PA0) 低->高触发
    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
        if(now - lastTick_PA0 > 50) {
            lastTick_PA0 = now;
            key_state = KEY_UP;
        }
    }

    // KEY_LEFT (PE2)
    if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_RESET) {
        if(now - lastTick_PE2 > 50) {
            lastTick_PE2 = now;
            key_state = KEY_LEFT;
        }
    }

    // KEY_DOWN (PE3)
    if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3) == GPIO_PIN_RESET) {
        if(now - lastTick_PE3 > 50) {
            lastTick_PE3 = now;
            key_state = KEY_DOWM;
        }
    }

    // KEY_RIGHT (PE4)
    if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET) {
        if(now - lastTick_PE4 > 50) {
            lastTick_PE4 = now;
            key_state = KEY_RIGHT;
        }
    }
}







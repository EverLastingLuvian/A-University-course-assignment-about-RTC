#ifndef __TETRISGAME_H
#define __TETRISGAME_H

#include "main.h"
#include "lcd.h"
#include "Menu.h"
#include "string.h"
#include <stdlib.h>

// ================= 配置 =================
#define MAP_W 10
#define MAP_H 20
#define BLOCK_SIZE 12  // 每个方格像素大小

// ================= 数据结构 =================
typedef struct {
    int x, y;             // 方块左上角坐标
    int type;             // 方块类型
    uint8_t shape[4][4];  // 当前方块形状
} Block;

// ================= 全局变量 =================
extern uint8_t map[MAP_H][MAP_W]; // 游戏区域
extern Block current;             // 当前方块

// ================= 函数声明 =================

/**
 * @brief 俄罗斯方块主循环
 *        KEY_RIGHT -> 顺时针旋转
 *        KEY_LEFT  -> 逆时针旋转
 *        KEY_DOWM  -> 快速下落
 *        KEY_UP    -> 退出游戏
 */
void tetrisGame(void);

/**
 * @brief 绘制游戏界面（固定方块 + 当前方块）
 */
void drawGame(void);

/**
 * @brief 固定当前方块到地图
 */
void lockBlock(void);

/**
 * @brief 消除满行
 */
void clearLines(void);

/**
 * @brief 生成一个新的方块
 */
void newBlock(void);

/**
 * @brief 顺时针旋转方块
 */
void rotateCW(uint8_t shape[4][4]);


/**
 * @brief 检查方块是否碰撞
 * @param nx 方块左上角X坐标
 * @param ny 方块左上角Y坐标
 * @param shape 方块形状
 * @return 1碰撞，0未碰撞
 */
int checkCollision(int nx, int ny, uint8_t shape[4][4]);




void Key_Scan(void);

#endif /* __TETRISGAME_H */

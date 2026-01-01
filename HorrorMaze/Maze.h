#pragma once
#include <vector>
#include <string>
#include <SFML/Graphics.hpp>

/**
 * Maze类：管理迷宫地图数据和渲染
 *
 * 地图格式：
 * 0 = 空地（可以走）
 * 1 = 墙（不能穿过）
 * 2 = 出口（胜利点）
 * 3 = 双胞胎位置
 * 4 = 雪墙（可躲藏）
 * 5 = 打火机道具
 */
class Maze {
public:
    Maze();

    // 从文件加载地图
    bool loadFromFile(const std::string& filename);

    // 地图查询函数
    bool isWall(int x, int y) const;           // 检查某个位置是否是墙
    int getCell(int x, int y) const;           // 获取某个格子的类型
    void setCell(int x, int y, int value);     // 设置某个格子的类型

    // 获取地图信息
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    sf::Vector2i getPlayerStart() const { return playerStart; }
    sf::Vector2i getExitPos() const { return exitPos; }

    // 渲染迷宫（俯视图）
    void renderTopDown(sf::RenderWindow& window, float cellSize) const;

private:
    int width;                              // 地图宽度
    int height;                             // 地图高度
    std::vector<std::vector<int>> map;      // 地图数据（二维数组）
    sf::Vector2i playerStart;               // 玩家起点
    sf::Vector2i exitPos;                   // 出口位置
};

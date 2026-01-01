#include "Maze.h"
#include <fstream>
#include <iostream>

Maze::Maze()
    : width(0)
    , height(0)
    , playerStart(1, 1)
    , exitPos(0, 0)
{
}

/**
 * 从文本文件加载地图
 *
 * 文件格式：
 * 第一行：宽度 高度
 * 后续行：地图数据（用空格分隔的数字）
 */
bool Maze::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open map file: " << filename << std::endl;
        return false;
    }

    // 读取宽度和高度
    file >> width >> height;

    std::cout << "Map size: " << width << " x " << height << std::endl;

    // 初始化地图（创建二维数组）
    map.resize(height);
    for (int y = 0; y < height; y++) {
        map[y].resize(width);
    }

    // 读取地图数据
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            file >> map[y][x];

            // 记录特殊位置
            if (map[y][x] == 2) {  // 出口
                exitPos = sf::Vector2i(x, y);
                std::cout << "Exit found at: (" << x << ", " << y << ")" << std::endl;
            }
        }
    }

    // 寻找玩家起点（第一个空地）
    bool foundStart = false;
    for (int y = 0; y < height && !foundStart; y++) {
        for (int x = 0; x < width && !foundStart; x++) {
            if (map[y][x] == 0) {  // 空地
                playerStart = sf::Vector2i(x, y);
                foundStart = true;
                std::cout << "Player start: (" << x << ", " << y << ")" << std::endl;
            }
        }
    }

    file.close();
    std::cout << "Map loaded successfully!" << std::endl;
    return true;
}

/**
 * 检查某个位置是否是墙
 */
bool Maze::isWall(int x, int y) const {
    // 边界外视为墙
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return true;
    }

    // 1 = 墙
    return map[y][x] == 1;
}

/**
 * 获取某个格子的类型
 */
int Maze::getCell(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 1;  // 边界外返回墙
    }
    return map[y][x];
}

/**
 * 设置某个格子的类型
 */
void Maze::setCell(int x, int y, int value) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        map[y][x] = value;
    }
}

/**
 * 渲染迷宫（俯视图）
 *
 * cellSize: 每个格子的像素大小
 */
void Maze::renderTopDown(sf::RenderWindow& window, float cellSize) const {
    sf::RectangleShape cell(sf::Vector2f(cellSize, cellSize));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            cell.setPosition({x * cellSize, y * cellSize});

            // 根据格子类型设置颜色
            switch (map[y][x]) {
                case 0:  // 空地
                    cell.setFillColor(sf::Color(50, 50, 50));
                    break;

                case 1:  // 墙
                    cell.setFillColor(sf::Color(200, 200, 200));
                    break;

                case 2:  // 出口
                    cell.setFillColor(sf::Color::Green);
                    break;

                case 3:  // 双胞胎
                    cell.setFillColor(sf::Color::Magenta);
                    break;

                case 4:  // 雪墙
                    cell.setFillColor(sf::Color::Cyan);
                    break;

                case 5:  // 打火机
                    cell.setFillColor(sf::Color::Yellow);
                    break;

                default:
                    cell.setFillColor(sf::Color::Black);
            }

            window.draw(cell);

            // 绘制网格线（方便看清格子）
            cell.setFillColor(sf::Color::Transparent);
            cell.setOutlineColor(sf::Color(80, 80, 80));
            cell.setOutlineThickness(1.0f);
            window.draw(cell);
        }
    }
}

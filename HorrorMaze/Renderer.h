#pragma once
#include <SFML/Graphics.hpp>
#include "Player.h"
#include "Maze.h"

/**
 * Renderer类：负责渲染游戏画面
 *
 * 支持两种渲染模式：
 * 1. 俯视图（TopDown）- 2D视角
 * 2. 第一人称（FirstPerson）- 3D视角（光线投射）
 */
class Renderer {
public:
    Renderer(int screenWidth, int screenHeight);

    // 渲染第一人称视角（3D - 光线投射）
    void renderFirstPerson(sf::RenderWindow& window,
                          const Player& player,
                          const Maze& maze,
                          const std::vector<sf::Vector2i>& escapePath = {});

    // 渲染俯视图（2D）
    void renderTopDown(sf::RenderWindow& window,
                      const Player& player,
                      const Maze& maze);

    // 获取深度缓冲（用于sprite深度测试）
    const std::vector<float>& getZBuffer() const { return zBuffer; }

private:
    int screenWidth;
    int screenHeight;

    // 雪墙纹理
    sf::Texture snowWallTexture;
    sf::Image snowWallImage;  // 用于像素级访问

    // 出口纹理
    sf::Texture exitTexture;
    sf::Image exitImage;  // 用于像素级访问

    // 深度缓冲（Z-Buffer）- 记录每列的墙壁距离
    std::vector<float> zBuffer;

    // 光线投射核心函数
    void castRays(sf::RenderWindow& window,
                 const Player& player,
                 const Maze& maze,
                 const std::vector<sf::Vector2i>& escapePath);

    // 辅助函数：根据墙的方向、距离和光照计算颜色
    sf::Color getWallColor(int side, float distance, const Player& player, float rayAngle);

    // 光照辅助函数
    float calculateConeEffect(float rayAngle);  // 计算锥形光束效果
    sf::Color applyLighterTint(const sf::Color& baseColor, float brightness);  // 应用打火机色调
    sf::Color applyGrayscale(const sf::Color& baseColor, float brightness);    // 应用灰度（打火机关闭时）
};

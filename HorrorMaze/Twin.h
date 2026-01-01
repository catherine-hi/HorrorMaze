#pragma once
#include <SFML/Graphics.hpp>
#include <string>

class Player;

/**
 * Twin类：双胞胎陷阱
 *
 * 功能：
 * 1. 固定位置生成（在迷宫中标记的位置）
 * 2. 玩家触碰时冻结4秒
 * 3. 发出声音吸引附近的鬼
 */
class Twin {
public:
    Twin(float x, float y);

    // 检查玩家是否触碰双胞胎
    bool checkCollision(const Player& player) const;

    // 激活双胞胎陷阱（冻结玩家+发出声音）
    // activate with custom duration (seconds). If duration <= 0, uses default SOUND_DURATION
    void activate(float duration = -1.0f);

    // 获取位置
    float getX() const { return x; }
    float getY() const { return y; }

    // 获取声音强度（用于吸引鬼）
    float getSoundLevel() const;

    // 是否已被触发
    bool isActivated() const { return activated; }

    // 更新状态（声音衰减）
    void update(float deltaTime);

    // 渲染（俯视图）
    void renderTopDown(sf::RenderWindow& window, float cellSize) const;

    // 渲染（第一人称）
    void renderFirstPerson(sf::RenderWindow& window, const Player& player,
                          int screenWidth, int screenHeight,
                          const std::vector<float>& zBuffer) const;

    // 加载sprite纹理（所有双胞胎共享）
    static bool loadSpriteTexture(const std::string& filename);

    // 获取默认声音持续时长
    static float getDefaultSoundDuration() { return SOUND_DURATION; }

private:
    float x, y;  // 双胞胎位置（格子中心）
    bool activated;  // 是否已被触发
    float soundTimer;  // 声音持续计时器（剩余时间）
    float initialSoundDuration; // 激活时的持续时长，用于衰减比例计算

    // 静态纹理（所有双胞胎共享）
    static sf::Texture s_spriteTexture;
    static bool s_textureLoaded;

    static constexpr float COLLISION_RADIUS = 0.6f;  // 触发半径
    static constexpr float SOUND_DURATION = 4.0f;  // 声音持续4秒（和冻结时间一致）
    static constexpr float SOUND_LEVEL = 2000.0f;  // 声音强度（适中，能有效吸引鬼但不会过度覆盖）
};

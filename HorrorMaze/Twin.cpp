#include "Twin.h"
#include "Player.h"
#include <cmath>
#include <iostream>

// 静态成员初始化
sf::Texture Twin::s_spriteTexture;
bool Twin::s_textureLoaded = false;

Twin::Twin(float startX, float startY)
    : x(startX)
    , y(startY)
    , activated(false)
    , soundTimer(0.0f)
    , initialSoundDuration(0.0f)
{
    // 构造函数不打印消息，由Game统一打印
}

/**
 * 检查玩家是否触碰双胞胎
 */
bool Twin::checkCollision(const Player& player) const {
    if (activated) {
        return false;  // 已触发过，不再检测
    }

    float dx = player.getX() - x;
    float dy = player.getY() - y;
    float distance = std::sqrt(dx * dx + dy * dy);

    return distance < COLLISION_RADIUS;
}

/**
 * 激活双胞胎陷阱
 */
void Twin::activate(float duration) {
    if (!activated) {
        activated = true;
        if (duration > 0.0f) {
            soundTimer = duration;
        } else {
            soundTimer = SOUND_DURATION;
        }
        initialSoundDuration = soundTimer;
        std::cout << "\n>>> TWIN TRAP ACTIVATED! <<<" << std::endl;
        std::cout << "Player frozen for " << soundTimer << " seconds!" << std::endl;
        std::cout << "Emitting loud sound to attract ghosts..." << std::endl;
    }
}

/**
 * 获取当前声音强度（随时间衰减）
 */
float Twin::getSoundLevel() const {
    if (!activated || soundTimer <= 0.0f) {
        return 0.0f;
    }

    // 声音随时间线性衰减（使用初始持续时间作为基准）
    float base = (initialSoundDuration > 0.0f) ? initialSoundDuration : SOUND_DURATION;
    float ratio = soundTimer / base;
    return SOUND_LEVEL * ratio;
}

/**
 * 更新状态（声音衰减，不重置）
 */
void Twin::update(float deltaTime) {
    if (activated && soundTimer > 0.0f) {
        soundTimer -= deltaTime;
        if (soundTimer <= 0.0f) {
            soundTimer = 0.0f;
            // 注意：activated保持为true，本局不再重置
            std::cout << "Twin trap sound ended - trap remains disabled for this game." << std::endl;
        }
    }
}

/**
 * 渲染双胞胎（俯视图）
 */
void Twin::renderTopDown(sf::RenderWindow& window, float cellSize) const {
    // 保持在激活期间仍然渲染（图像在声效结束后消失）
    // 如果已经激活但声音结束，则隐藏
    if (activated && soundTimer <= 0.0f) {
        return;
    }

    // 绘制双胞胎方块（未触发时为紫色）
    float size = cellSize * 0.6f;
    sf::RectangleShape twinSquare({size, size});
    twinSquare.setFillColor(sf::Color(150, 50, 150));  // 紫色（醒目）

    twinSquare.setOrigin({size / 2, size / 2});
    twinSquare.setPosition({x * cellSize, y * cellSize});

    window.draw(twinSquare);
}

/**
 * 加载双胞胎sprite纹理（所有双胞胎共享）
 */
bool Twin::loadSpriteTexture(const std::string& filename) {
    if (s_textureLoaded) {
        return true;  // 已经加载过了
    }

    if (s_spriteTexture.loadFromFile(filename)) {
        s_textureLoaded = true;
        std::cout << "Twin sprite loaded: " << filename << std::endl;
        return true;
    }

    std::cerr << "Failed to load twin sprite: " << filename << std::endl;
    return false;
}

/**
 * 渲染双胞胎（第一人称）
 * 使用sprite纹理渲染（如果已加载）
 */
void Twin::renderFirstPerson(sf::RenderWindow& window, const Player& player,
                             int screenWidth, int screenHeight,
                             const std::vector<float>& zBuffer) const {
    // 保持在激活期间仍然渲染（图像在声效结束后消失）
    if (activated && soundTimer <= 0.0f) {
        return;
    }

    // 计算相对位置
    float spriteX = x - player.getX();
    float spriteY = y - player.getY();

    // 变换到相机坐标系
    float invDet = 1.0f / (player.getPlaneX() * player.getDirY() - player.getDirX() * player.getPlaneY());
    float transformX = invDet * (player.getDirY() * spriteX - player.getDirX() * spriteY);
    float transformY = invDet * (-player.getPlaneY() * spriteX + player.getPlaneX() * spriteY);

    // 检查是否在玩家前方
    if (transformY <= 0.1f) {
        return;
    }

    // 计算屏幕位置
    int spriteScreenX = static_cast<int>((screenWidth / 2) * (1 + transformX / transformY));
    int spriteHeight = static_cast<int>(screenHeight / transformY * 0.5f);
    int spriteWidth = spriteHeight;

    float horizon = screenHeight * 0.5f + player.getCameraOffsetY();
    if (horizon < 1.0f) {
        horizon = 1.0f;
    }
    if (horizon > screenHeight - 1.0f) {
        horizon = screenHeight - 1.0f;
    }
    int horizonY = static_cast<int>(horizon);

    // 调整垂直位置
    int drawStartY = -spriteHeight / 2 + horizonY;
    if (drawStartY < 0) drawStartY = 0;
    int drawEndY = spriteHeight / 2 + horizonY;
    if (drawEndY >= screenHeight) drawEndY = screenHeight - 1;

    int drawStartX = -spriteWidth / 2 + spriteScreenX;
    if (drawStartX < 0) drawStartX = 0;
    int drawEndX = spriteWidth / 2 + spriteScreenX;
    if (drawEndX >= screenWidth) drawEndX = screenWidth - 1;

    // 如果加载了纹理，使用sprite渲染
    if (s_textureLoaded) {
        sf::Vector2u texSize = s_spriteTexture.getSize();

        for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
            // 深度测试
            if (transformY >= zBuffer[stripe]) {
                continue;
            }

            // 计算纹理X坐标
            int texX = static_cast<int>((stripe - (-spriteWidth / 2 + spriteScreenX)) * texSize.x / spriteWidth);

            // 绘制一列sprite
            sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);

            float texLeft = static_cast<float>(texX);
            float texRight = texLeft + 1.0f;

            // 根据距离调整亮度
            float brightness = 1.0f / (1.0f + transformY * 0.1f);
            brightness = std::max(0.3f, std::min(1.0f, brightness));

            std::uint8_t colorValue = static_cast<std::uint8_t>(255 * brightness);
            sf::Color lightColor(colorValue, colorValue, colorValue);

            // 设置4个顶点
            quad[0].position = sf::Vector2f(static_cast<float>(stripe), static_cast<float>(drawStartY));
            quad[0].texCoords = sf::Vector2f(texLeft, 0.0f);
            quad[0].color = lightColor;

            quad[1].position = sf::Vector2f(static_cast<float>(stripe + 1), static_cast<float>(drawStartY));
            quad[1].texCoords = sf::Vector2f(texRight, 0.0f);
            quad[1].color = lightColor;

            quad[2].position = sf::Vector2f(static_cast<float>(stripe), static_cast<float>(drawEndY));
            quad[2].texCoords = sf::Vector2f(texLeft, static_cast<float>(texSize.y));
            quad[2].color = lightColor;

            quad[3].position = sf::Vector2f(static_cast<float>(stripe + 1), static_cast<float>(drawEndY));
            quad[3].texCoords = sf::Vector2f(texRight, static_cast<float>(texSize.y));
            quad[3].color = lightColor;

            sf::RenderStates states;
            states.texture = &s_spriteTexture;
            window.draw(quad, states);
        }
    } else {
        // 没有纹理，使用彩色方块（紫色）
        sf::RectangleShape twinSprite({
            static_cast<float>(drawEndX - drawStartX),
            static_cast<float>(drawEndY - drawStartY)
        });

        twinSprite.setFillColor(sf::Color(150, 50, 150, 200));  // 紫色
        twinSprite.setPosition({static_cast<float>(drawStartX), static_cast<float>(drawStartY)});

        // 深度测试（简化版：只检查中心点）
        if (spriteScreenX >= 0 && spriteScreenX < screenWidth) {
            if (transformY < zBuffer[spriteScreenX]) {
                window.draw(twinSprite);
            }
        }
    }
}

#include "Renderer.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <stdexcept>

Renderer::Renderer(int w, int h)
    : screenWidth(w)
    , screenHeight(h)
    , zBuffer(w, 0.0f)  // 初始化深度缓冲
{
    std::cout << "Renderer initialized: " << w << "x" << h << std::endl;

    // 加载雪墙纹理
    std::vector<std::string> texturePaths = {
        "assets/textures/snow_wall.png",
        "../../assets/textures/snow_wall.png",
        "E:/cs106A data structures/Final_Project/HorrorMazeFinal/assets/textures/snow_wall.png"
    };

    bool textureLoaded = false;
    for (const auto& path : texturePaths) {
        if (snowWallImage.loadFromFile(path)) {
            std::cout << "Snow wall texture loaded from: " << path << std::endl;
            if (snowWallTexture.loadFromImage(snowWallImage)) {
                textureLoaded = true;
                break;
            }
        }
    }

    if (!textureLoaded) {
        throw std::runtime_error("Required texture missing: snow_wall.png");
    }

    // 加载出口纹理
    std::vector<std::string> exitTexturePaths = {
        "assets/textures/exit.png",
        "../../assets/textures/exit.png",
        "E:/cs106A data structures/Final_Project/HorrorMazeFinal/assets/textures/exit.png"
    };

    bool exitTextureLoaded = false;
    for (const auto& path : exitTexturePaths) {
        if (exitImage.loadFromFile(path)) {
            std::cout << "Exit texture loaded from: " << path << std::endl;
            if (exitTexture.loadFromImage(exitImage)) {
                exitTextureLoaded = true;
                break;
            }
        }
    }

    if (!exitTextureLoaded) {
        std::cerr << "WARNING: Exit texture (exit.png) not found! Using default wall texture." << std::endl;
        // 不抛出异常，使用默认墙壁纹理作为备用
        exitTexture = snowWallTexture;
        exitImage = snowWallImage;
    }
}

/**
 * 渲染第一人称视角（3D）
 *
 * 流程：
 * 1. 绘制天空和地板
 * 2. 使用光线投射绘制墙壁
 * 3. 如果激活闪灵，应用特殊视觉效果
 */
void Renderer::renderFirstPerson(sf::RenderWindow& window,
                                 const Player& player,
                                 const Maze& maze,
                                 const std::vector<sf::Vector2i>& escapePath) {
    bool spiritVisionActive = player.isSpiritVisionActive();
    float horizon = screenHeight * 0.5f + player.getCameraOffsetY();
    if (horizon < 1.0f) {
        horizon = 1.0f;
    }
    if (horizon > screenHeight - 1.0f) {
        horizon = screenHeight - 1.0f;
    }
    int horizonY = static_cast<int>(horizon);

    // 步骤1：绘制天空渐变（上半部分）
    for (int y = 0; y < horizonY; y++) {
        float gradient = (horizonY > 0) ? static_cast<float>(y) / horizonY : 0.0f;

        // 正常模式：从深灰 (50, 50, 55) 到更深的灰 (40, 40, 45)
        // 注意：地平线要和地板衔接（都是黑色）
        int r = static_cast<int>(50 - gradient * 50);  // 50 → 0
        int g = static_cast<int>(50 - gradient * 50);  // 50 → 0
        int b = static_cast<int>(55 - gradient * 55);  // 55 → 0

        // 闪灵模式：反转颜色 + 添加诡异的暗红色偏色 + 晕影
        if (spiritVisionActive) {
            r = 255 - r;
            g = 255 - g;
            b = 255 - b;

            // 降低整体亮度（深灰色调）- 和地板一致
            r = static_cast<int>(r * 0.4f);
            g = static_cast<int>(g * 0.4f);
            b = static_cast<int>(b * 0.4f);

            // 添加暗红色偏色（血腥恐怖感）- 和地板一致
            r = std::min(255, static_cast<int>(r * 1.1f));   // 轻微增强红色
            g = static_cast<int>(g * 0.5f);   // 中度减弱绿色
            b = static_cast<int>(b * 0.4f);   // 中度减弱蓝色

            // 轻微增强对比度
            float contrast = 1.15f;
            r = static_cast<int>((r - 128) * contrast + 128);
            g = static_cast<int>((g - 128) * contrast + 128);
            b = static_cast<int>((b - 128) * contrast + 128);

            // 晕影效果 - 和地板一致
            float vignette = 1.0f - std::abs(gradient - 0.5f) * 0.3f;
            r = static_cast<int>(r * vignette);
            g = static_cast<int>(g * vignette);
            b = static_cast<int>(b * vignette);

            // 天空额外亮度提升（地平线处=1.0，顶部=1.6倍）
            float skyBrightness = 1.0f + (1.0f - gradient) * 0.6f;
            r = static_cast<int>(r * skyBrightness);
            g = static_cast<int>(g * skyBrightness);
            b = static_cast<int>(b * skyBrightness);

            r = std::min(255, std::max(0, r));
            g = std::min(255, std::max(0, g));
            b = std::min(255, std::max(0, b));
        }

        sf::RectangleShape skyLine({static_cast<float>(screenWidth), 1.0f});
        skyLine.setFillColor(sf::Color(r, g, b));
        skyLine.setPosition({0, static_cast<float>(y)});
        window.draw(skyLine);
    }

    // 步骤2：绘制地板渐变（下半部分）
    float floorDenom = screenHeight - horizon;
    if (floorDenom < 1.0f) {
        floorDenom = 1.0f;
    }
    for (int y = horizonY; y < screenHeight; y++) {
        // distanceFactor: 0（地平线/远）到 1（脚下/近）
        float distanceFactor = (static_cast<float>(y) - horizon) / floorDenom;

        int r, g, b;

        if (player.isLighterOn()) {
            // 打火机开启：白色/灰色调（匹配雪墙），近处亮 → 远处黑色
            if (distanceFactor > 0.6f) {
                // 近处（distanceFactor > 0.6）：灰白色 (180, 180, 180)
                float t = (distanceFactor - 0.6f) / 0.4f;
                r = static_cast<int>(57 + t * 123);   // 57 → 180
                g = static_cast<int>(55 + t * 125);   // 55 → 180
                b = static_cast<int>(50 + t * 130);   // 50 → 180
            } else {
                // 远处（distanceFactor < 0.6）：黑色 (0, 0, 0) → 深灰色 (57, 55, 50)
                float t = distanceFactor / 0.6f;
                r = static_cast<int>(57 * t);
                g = static_cast<int>(55 * t);
                b = static_cast<int>(50 * t);
            }
        } else {
            // 打火机关闭：近处深灰 → 远处黑色
            // 近处深灰 (35)，远处黑色 (0)
            int gray = static_cast<int>(35 * distanceFactor);
            r = g = b = gray;
        }

        // 闪灵模式：反转颜色 + 添加诡异的暗红色偏色 + 边缘晕影
        if (spiritVisionActive) {
            r = 255 - r;
            g = 255 - g;
            b = 255 - b;

            // 降低整体亮度（深灰色调）
            r = static_cast<int>(r * 0.4f);
            g = static_cast<int>(g * 0.4f);
            b = static_cast<int>(b * 0.4f);

            // 添加暗红色偏色（血腥恐怖感）
            r = std::min(255, static_cast<int>(r * 1.1f));   // 轻微增强红色
            g = static_cast<int>(g * 0.5f);   // 中度减弱绿色
            b = static_cast<int>(b * 0.4f);   // 中度减弱蓝色

            // 轻微增强对比度
            float contrast = 1.15f;
            r = static_cast<int>((r - 128) * contrast + 128);
            g = static_cast<int>((g - 128) * contrast + 128);
            b = static_cast<int>((b - 128) * contrast + 128);

            // 边缘晕影效果（让边缘更暗）
            float vignette = 1.0f - std::abs(distanceFactor - 0.5f) * 0.3f;
            r = static_cast<int>(r * vignette);
            g = static_cast<int>(g * vignette);
            b = static_cast<int>(b * vignette);

            r = std::min(255, std::max(0, r));
            g = std::min(255, std::max(0, g));
            b = std::min(255, std::max(0, b));
        }

        sf::RectangleShape floorLine({static_cast<float>(screenWidth), 1.0f});
        floorLine.setFillColor(sf::Color(r, g, b));
        floorLine.setPosition({0, static_cast<float>(y)});
        window.draw(floorLine);
    }

    // 步骤3：光线投射 - 绘制墙壁
    castRays(window, player, maze, escapePath);

    // 步骤4：如果闪灵激活，在地板上绘制荧光蓝路径光斑
    if (spiritVisionActive && !escapePath.empty()) {
        // 获取玩家信息
        float posX = player.getX();
        float posY = player.getY();
        float dirX = player.getDirX();
        float dirY = player.getDirY();
        float planeX = player.getPlaneX();
        float planeY = player.getPlaneY();

        // 用于避免路径标记重叠的已绘制位置集合
        std::vector<sf::Vector2i> drawnPositions;
        const int MIN_SPACING = 25;  // 最小间距（像素）

        // 遍历所有路径格子
        for (const auto& pathCell : escapePath) {
            // 计算路径格子中心相对于玩家的位置
            float relX = pathCell.x + 0.5f - posX;
            float relY = pathCell.y + 0.5f - posY;

            // 转换到相机空间
            float invDet = 1.0f / (planeX * dirY - dirX * planeY);
            float transformX = invDet * (dirY * relX - dirX * relY);
            float transformY = invDet * (-planeY * relX + planeX * relY);

            // 如果在视野内且在前方（距离合理）
            if (transformY > 0.5f && transformY < 20.0f) {
                // 计算在屏幕上的X位置
                int screenX = static_cast<int>((screenWidth / 2) * (1 + transformX / transformY));

                // 检查是否在屏幕范围内
                if (screenX >= 0 && screenX < screenWidth) {
                    // === 深度测试：检查是否被墙遮挡 ===
                    // 只有当路径点比墙更近时才绘制
                    if (transformY < zBuffer[screenX]) {
                        // 计算在屏幕上的Y位置（地板投影）
                        int screenY = static_cast<int>(horizon + (screenHeight / 2.0f) / transformY);

                        // 限制在屏幕下半部分
                        if (screenY >= horizonY && screenY < screenHeight + 50) {
                            // === 检查是否与已绘制的标记距离过近 ===
                            bool tooClose = false;
                            for (const auto& drawn : drawnPositions) {
                                int dx = screenX - drawn.x;
                                int dy = screenY - drawn.y;
                                if (dx*dx + dy*dy < MIN_SPACING*MIN_SPACING) {
                                    tooClose = true;
                                    break;
                                }
                            }

                            if (tooClose) {
                                continue;  // 跳过这个标记
                            }

                            // 记录这个位置
                            drawnPositions.push_back({screenX, screenY});

                            // 计算光斑大小（近大远小，稍微缩小一点）
                            float spotRadius = (screenHeight / transformY) * 0.15f;
                            spotRadius = std::max(6.0f, std::min(100.0f, spotRadius));

                            // 计算亮度（距离衰减）
                            float brightness = 1.0f / (1.0f + transformY * 0.15f);
                            brightness = std::max(0.4f, std::min(1.0f, brightness));

                            // 绘制荧光蓝色光斑（贴地）
                            sf::CircleShape spot(spotRadius);
                            spot.setOrigin({spotRadius, spotRadius});
                            spot.setPosition({static_cast<float>(screenX), static_cast<float>(screenY)});

                            // 荧光蓝色
                            int r = static_cast<int>(20 * brightness);
                            int g = std::min(255, static_cast<int>(180 * brightness));
                            int b = std::min(255, static_cast<int>(255 * brightness));

                            spot.setFillColor(sf::Color(r, g, b, 80));  // 降低透明度
                            window.draw(spot);  // 使用默认混合（BlendAlpha）
                        }
                    }
                }
            }
        }
    }
}

/**
 * 光线投射核心算法（DDA - Digital Differential Analyzer）
 *
 * 原理：
 * - 对屏幕的每一列投射一条光线
 * - 光线沿着方向前进，直到碰到墙
 * - 根据距离计算墙的高度
 * - 绘制垂直条带
 * - 如果闪灵激活，应用特殊视觉效果
 */
void Renderer::castRays(sf::RenderWindow& window,
                       const Player& player,
                       const Maze& maze,
                       const std::vector<sf::Vector2i>& escapePath) {
    bool spiritVisionActive = player.isSpiritVisionActive();
    float horizon = screenHeight * 0.5f + player.getCameraOffsetY();
    if (horizon < 1.0f) {
        horizon = 1.0f;
    }
    if (horizon > screenHeight - 1.0f) {
        horizon = screenHeight - 1.0f;
    }

    // 辅助函数：检查格子是否在逃生路径上
    auto isInPath = [&escapePath](int x, int y) -> bool {
        for (const auto& pos : escapePath) {
            if (pos.x == x && pos.y == y) {
                return true;
            }
        }
        return false;
    };
    // 获取玩家信息
    float posX = player.getX();
    float posY = player.getY();
    float dirX = player.getDirX();
    float dirY = player.getDirY();
    float planeX = player.getPlaneX();
    float planeY = player.getPlaneY();

    // 对屏幕的每一列投射一条光线
    for (int x = 0; x < screenWidth; x++) {
        // === 1. 计算光线方向 ===

        // cameraX: 当前列在屏幕上的归一化位置 [-1, 1]
        // -1 = 屏幕最左边, 0 = 屏幕中间, 1 = 屏幕最右边
        float cameraX = 2.0f * x / static_cast<float>(screenWidth) - 1.0f;

        // 光线方向 = 玩家朝向 + 相机平面 × 位置
        // 这样可以形成一个扇形的视野
        float rayDirX = dirX + planeX * cameraX;
        float rayDirY = dirY + planeY * cameraX;

        // === 2. 初始化DDA变量 ===

        // 玩家当前所在的地图格子
        int mapX = static_cast<int>(posX);
        int mapY = static_cast<int>(posY);

        // 光线每前进1格在X/Y方向上移动的距离
        // 使用三角函数计算：1 / cos(angle)
        float deltaDistX = (rayDirX == 0) ? 1e30f : std::abs(1.0f / rayDirX);
        float deltaDistY = (rayDirY == 0) ? 1e30f : std::abs(1.0f / rayDirY);

        // 光线前进的方向（-1 或 +1）
        int stepX, stepY;

        // 从当前位置到下一个格子边界的距离
        float sideDistX, sideDistY;

        // === 3. 计算步进方向和初始边界距离 ===

        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (posX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0f - posX) * deltaDistX;
        }

        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (posY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0f - posY) * deltaDistY;
        }

        // === 4. DDA算法 - 沿光线前进直到碰到墙 ===

        bool hit = false;  // 是否碰到墙
        int side = 0;      // 碰到的墙的方向（0=垂直墙, 1=水平墙）

        while (!hit) {
            // 选择距离更近的边界前进
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;  // 垂直墙（东西方向）
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;  // 水平墙（南北方向）
            }

            // 检查是否碰到墙
            if (maze.isWall(mapX, mapY)) {
                hit = true;
            }
        }

        // === 5. 计算墙的距离（重要：避免鱼眼效果！）===

        // 使用垂直距离，而不是真实距离
        // 这样可以避免图像边缘的扭曲（鱼眼效果）
        float perpWallDist;
        if (side == 0) {
            perpWallDist = (mapX - posX + (1 - stepX) / 2) / rayDirX;
        } else {
            perpWallDist = (mapY - posY + (1 - stepY) / 2) / rayDirY;
        }

        // 避免除以0
        if (perpWallDist < 0.01f) perpWallDist = 0.01f;

        // === 记录深度到Z-Buffer（用于sprite深度测试）===
        zBuffer[x] = perpWallDist;

        // === 6. 计算墙在屏幕上的高度 ===

        // 距离越近，墙越高
        int lineHeight = static_cast<int>(screenHeight / perpWallDist);

        // 计算墙在屏幕上的起始和结束Y坐标
        int drawStart = static_cast<int>(-lineHeight / 2.0f + horizon);
        if (drawStart < 0) drawStart = 0;

        int drawEnd = static_cast<int>(lineHeight / 2.0f + horizon);
        if (drawEnd >= screenHeight) drawEnd = screenHeight - 1;

        // === 7. 计算纹理坐标 ===

        // 检查当前墙格子是否是出口（cell value = 2）
        bool isExit = (maze.getCell(mapX, mapY) == 2);

        // 根据是否是出口选择纹理
        const sf::Image& wallImage = isExit ? exitImage : snowWallImage;
        const sf::Texture& wallTexture = isExit ? exitTexture : snowWallTexture;

        // 计算光线击中墙的精确位置（wallX）
        float wallX;  // 0.0 到 1.0 之间
        if (side == 0) {
            wallX = posY + perpWallDist * rayDirY;
        } else {
            wallX = posX + perpWallDist * rayDirX;
        }
        wallX -= std::floor(wallX);  // 只保留小数部分

        // 计算纹理的 X 坐标
        int texWidth = wallImage.getSize().x;
        int texHeight = wallImage.getSize().y;
        int texX = static_cast<int>(wallX * texWidth);
        if ((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0)) {
            texX = texWidth - texX - 1;
        }

        // === 8. 计算光照因子 ===

        float rayAngle = cameraX;  // 光线角度（用于打火机光照）

        // 计算光照亮度（但不应用颜色，让纹理本身的颜色显示）
        float brightness;
        if (!player.isLighterOn()) {
            // 打火机关闭：极暗环境光，远处墙与地面融为一体
            float ambientLight = 0.08f;   // 降低到 8%
            float fogFactor = 0.15f;      // 增强雾效果
            brightness = ambientLight / (1.0f + perpWallDist * fogFactor);
            brightness = std::max(0.03f, std::min(0.12f, brightness));  // 极暗范围
        } else {
            // 打火机开启：降低亮度和范围，匹配地板
            float distanceFactor = 1.0f / (1.0f + perpWallDist * 0.6f);  // 更快衰减
            float angleFactor = calculateConeEffect(rayAngle);
            brightness = distanceFactor * angleFactor;
            brightness = std::max(0.10f, std::min(0.45f, brightness));  // 降低最大亮度到 45%
        }

        // 侧面阴影：水平墙比垂直墙暗一些
        if (side == 1) {
            brightness *= 0.75f;
        }

        // 闪灵模式：反转亮度（负片效果）
        if (spiritVisionActive) {
            brightness = 1.0f - brightness;
        }

        // === 9. 高效渲染纹理墙条（使用 VertexArray 批量绘制）===

        // 创建顶点数组：4个顶点组成一个四边形
        sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);

        // 纹理坐标（使用整列纹理）
        float texLeft = static_cast<float>(texX);
        float texRight = texLeft + 1.0f;

        // 设置4个顶点的位置和纹理坐标
        // 顶点0：左上角
        quad[0].position = sf::Vector2f(static_cast<float>(x), static_cast<float>(drawStart));
        quad[0].texCoords = sf::Vector2f(texLeft, 0.0f);

        // 顶点1：右上角
        quad[1].position = sf::Vector2f(static_cast<float>(x + 1), static_cast<float>(drawStart));
        quad[1].texCoords = sf::Vector2f(texRight, 0.0f);

        // 顶点2：左下角
        quad[2].position = sf::Vector2f(static_cast<float>(x), static_cast<float>(drawEnd));
        quad[2].texCoords = sf::Vector2f(texLeft, static_cast<float>(texHeight));

        // 顶点3：右下角
        quad[3].position = sf::Vector2f(static_cast<float>(x + 1), static_cast<float>(drawEnd));
        quad[3].texCoords = sf::Vector2f(texRight, static_cast<float>(texHeight));

        // 计算光照颜色
        int r = static_cast<int>(255 * brightness);
        int g = static_cast<int>(255 * brightness);
        int b = static_cast<int>(255 * brightness);

        // 正常模式且打火机开启时添加微弱的橙黄色调
        if (!spiritVisionActive && player.isLighterOn()) {
            r = static_cast<int>(r * 1.10f);  // 只稍微增强暖色
            g = static_cast<int>(g * 1.05f);
            b = static_cast<int>(b * 0.90f);  // 只稍微减弱蓝色
        }

        // 闪灵模式：添加诡异的暗红色偏色 + 降低亮度
        if (spiritVisionActive) {
            // 降低整体亮度（深灰色调）
            r = static_cast<int>(r * 0.6f);
            g = static_cast<int>(g * 0.6f);
            b = static_cast<int>(b * 0.6f);

            // 添加暗红色偏色（血腥恐怖感）
            r = std::min(255, static_cast<int>(r * 1.1f));   // 轻微增强红色
            g = static_cast<int>(g * 0.5f);   // 中度减弱绿色
            b = static_cast<int>(b * 0.4f);   // 中度减弱蓝色

            // 轻微增强对比度
            float contrast = 1.15f;
            r = static_cast<int>((r - 128) * contrast + 128);
            g = static_cast<int>((g - 128) * contrast + 128);
            b = static_cast<int>((b - 128) * contrast + 128);
        }

        r = std::min(255, std::max(0, r));
        g = std::min(255, std::max(0, g));
        b = std::min(255, std::max(0, b));

        // 闪灵模式：墙壁不透明（避免地平线分界）
        int alpha = spiritVisionActive ? 180 : 255;

        sf::Color lightColor(r, g, b, alpha);

        // 应用光照到所有顶点
        for (int i = 0; i < 4; i++) {
            quad[i].color = lightColor;
        }

        // 一次性绘制整条墙（只需1次 draw call！）
        sf::RenderStates states;
        states.texture = &wallTexture;  // 使用根据出口检测选择的纹理
        window.draw(quad, states);
    }
}

/**
 * 根据墙的方向、距离和打火机光照获取颜色
 *
 * @param side 墙的方向（0=垂直, 1=水平）
 * @param distance 到墙的距离
 * @param player 玩家对象（用于获取打火机状态）
 * @param rayAngle 光线相对于视野中心的角度（-1到1）
 * @return 墙的颜色（应用光照效果）
 */
sf::Color Renderer::getWallColor(int side, float distance,
                                  const Player& player, float rayAngle) {
    // === 1. 基础颜色：根据墙的方向 ===
    sf::Color baseColor;

    if (side == 0) {
        // 垂直墙（东西方向）- 稍亮
        baseColor = sf::Color(180, 180, 180);
    } else {
        // 水平墙（南北方向）- 稍暗（制造阴影效果）
        baseColor = sf::Color(130, 130, 130);
    }

    // === 2. 打火机光照效果 ===

    float brightness;

    if (!player.isLighterOn()) {
        // 打火机关闭：使用旧版雾效果 + 极低环境光
        float ambientLight = 0.15f;   // 基础环境光（15%）
        float fogFactor = 0.12f;      // 雾的衰减系数

        // 距离雾效果：brightness = ambient / (1 + distance * fog)
        brightness = ambientLight / (1.0f + distance * fogFactor);

        // 限制亮度范围 [0.05, 0.25]（非常暗）
        brightness = std::max(0.05f, std::min(0.25f, brightness));

        // 使用灰度渲染（不添加橙黄色调）
        return applyGrayscale(baseColor, brightness);

    } else {
        // 打火机开启：计算动态光照

        // A. 距离衰减：离得越远越暗
        // 使用更快的衰减（打火机照射范围有限）
        float distanceFactor = 1.0f / (1.0f + distance * 0.4f);

        // B. 角度衰减：锥形光束效果
        // 视野中心最亮，边缘变暗
        float angleFactor = calculateConeEffect(rayAngle);

        // C. 综合亮度
        brightness = distanceFactor * angleFactor;

        // D. 限制亮度范围 [0.15, 1.0]
        brightness = std::max(0.15f, std::min(1.0f, brightness));

        // 应用打火机橙黄色调
        return applyLighterTint(baseColor, brightness);
    }
}

/**
 * 计算锥形光束效果（第一人称视角专用）
 *
 * @param rayAngle 光线角度（-1 = 最左边，0 = 中心，1 = 最右边）
 * @return 角度因子（0.0 到 1.0），中心最亮
 */
float Renderer::calculateConeEffect(float rayAngle) {
    // 使用平方函数创建平滑的锥形光束
    // rayAngle^2: 中心(0) = 0, 边缘(±1) = 1
    float angleSquared = rayAngle * rayAngle;

    // 反转：中心 = 1.0（最亮），边缘 = 0.0（最暗）
    float coneFactor = 1.0f - angleSquared * 0.7f;

    // 确保不会太暗
    return std::max(0.3f, coneFactor);
}

/**
 * 应用打火机色调和亮度（第一人称视角专用）
 *
 * @param baseColor 基础墙壁颜色
 * @param brightness 亮度因子（0.0 到 1.0）
 * @return 应用了打火机橙黄色调的颜色
 */
sf::Color Renderer::applyLighterTint(const sf::Color& baseColor, float brightness) {
    // 打火机的橙黄色调
    float orangeTint = 1.2f;   // 增强红色通道
    float yellowTint = 1.1f;   // 增强绿色通道
    float blueTint = 0.6f;     // 减弱蓝色通道（营造暖色调）

    // 应用亮度和色调
    int r = static_cast<int>(baseColor.r * brightness * orangeTint);
    int g = static_cast<int>(baseColor.g * brightness * yellowTint);
    int b = static_cast<int>(baseColor.b * brightness * blueTint);

    // 限制在 [0, 255] 范围内
    r = std::min(255, std::max(0, r));
    g = std::min(255, std::max(0, g));
    b = std::min(255, std::max(0, b));

    return sf::Color(
        static_cast<std::uint8_t>(r),
        static_cast<std::uint8_t>(g),
        static_cast<std::uint8_t>(b)
    );
}

/**
 * 应用灰度和亮度（打火机关闭时使用）
 *
 * @param baseColor 基础墙壁颜色
 * @param brightness 亮度因子（0.0 到 1.0）
 * @return 灰度调整后的颜色（不添加橙黄色调）
 */
sf::Color Renderer::applyGrayscale(const sf::Color& baseColor, float brightness) {
    // 直接应用亮度，保持灰色调（不添加打火机的橙黄色）
    int r = static_cast<int>(baseColor.r * brightness);
    int g = static_cast<int>(baseColor.g * brightness);
    int b = static_cast<int>(baseColor.b * brightness);

    // 限制在 [0, 255] 范围内
    r = std::min(255, std::max(0, r));
    g = std::min(255, std::max(0, g));
    b = std::min(255, std::max(0, b));

    return sf::Color(
        static_cast<std::uint8_t>(r),
        static_cast<std::uint8_t>(g),
        static_cast<std::uint8_t>(b)
    );
}

/**
 * 渲染俯视图（2D）
 *
 * 直接调用Maze和Player的渲染函数
 */
void Renderer::renderTopDown(sf::RenderWindow& window,
                            const Player& player,
                            const Maze& maze) {
    // 计算每个格子的大小
    float cellSize = std::min(
        screenWidth / static_cast<float>(maze.getWidth()),
        screenHeight / static_cast<float>(maze.getHeight())
    );

    // 调用Maze和Player的渲染函数
    maze.renderTopDown(window, cellSize);
    player.renderTopDown(window, cellSize);
}

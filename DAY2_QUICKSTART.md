# 🚀 Day 2 快速开始指南 - 3D渲染（光线投射）

## 📋 今天的目标

**第2天（Day 2）**：实现3D第一人称渲染，让游戏从俯视图变成真正的3D迷宫！

**预期效果**：
- 按 Tab 键切换俯视图/第一人称视角
- 第一人称中看到3D墙壁（类似DOOM游戏）
- WASD控制移动和转向，体验真实的3D迷宫探索

---

## 🎯 核心技术：光线投射（Raycasting）

### **什么是光线投射？**

光线投射是一种**伪3D**渲染技术，用于创建类似《DOOM》《德军总部3D》的第一人称游戏。

**原理**：
```
玩家视角
    |
    | 发射很多条光线
    |/|/|/|/|/|
    墙墙墙墙墙墙

每条光线：
1. 从玩家位置出发
2. 沿着视线方向前进
3. 检测是否碰到墙
4. 根据距离计算墙的高度
5. 在屏幕上绘制一条垂直线
```

**为什么叫"伪3D"？**
- 真3D：每个物体有x, y, z坐标
- 伪3D：只有x, y坐标（2D地图），但渲染成3D效果

---

## 📚 必读教程

**强烈推荐**：Lode's Raycasting Tutorial
- 网址：https://lodev.org/cgtutor/raycasting.html
- 这是**最经典、最详细**的光线投射教程
- 有完整的C++代码和详细解释
- 建议先花30分钟阅读理解原理

---

## 💻 今天的任务清单

### **任务1：创建Renderer类**（1小时）
- [ ] 创建 `Renderer.h` 和 `Renderer.cpp`
- [ ] 定义基本的渲染接口

### **任务2：实现DDA算法**（2小时）
- [ ] DDA = Digital Differential Analyzer（数字微分分析器）
- [ ] 用于快速检测光线与墙的交点
- [ ] 这是光线投射的核心算法

### **任务3：渲染墙壁**（2小时）
- [ ] 计算墙的高度（距离越近越高）
- [ ] 在屏幕上绘制垂直条带
- [ ] 测试基本的3D效果

### **任务4：优化和调试**（1小时）
- [ ] 修复鱼眼效果（重要！）
- [ ] 调整性能（目标60 FPS）
- [ ] 添加不同颜色的墙（区分东西南北）

---

## 🔧 实现步骤详解

### **步骤1：创建Renderer类框架**

创建 `Renderer.h`：

```cpp
#pragma once
#include <SFML/Graphics.hpp>
#include "Player.h"
#include "Maze.h"

/**
 * Renderer类：负责渲染游戏画面
 *
 * 支持两种渲染模式：
 * 1. 俯视图（TopDown）- 已在Maze和Player类中实现
 * 2. 第一人称（FirstPerson）- 使用光线投射
 */
class Renderer {
public:
    Renderer(int screenWidth, int screenHeight);

    // 渲染第一人称视角（光线投射）
    void renderFirstPerson(sf::RenderWindow& window,
                          const Player& player,
                          const Maze& maze);

    // 渲染俯视图（调用Maze和Player的渲染）
    void renderTopDown(sf::RenderWindow& window,
                      const Player& player,
                      const Maze& maze);

private:
    int screenWidth;
    int screenHeight;

    // 光线投射核心函数
    void castRays(sf::RenderWindow& window,
                 const Player& player,
                 const Maze& maze);

    // 辅助函数
    sf::Color getWallColor(int side);  // 根据墙的朝向返回颜色
};
```

---

### **步骤2：光线投射核心算法**

创建 `Renderer.cpp`：

```cpp
#include "Renderer.h"
#include <cmath>
#include <iostream>

Renderer::Renderer(int w, int h)
    : screenWidth(w)
    , screenHeight(h)
{
    std::cout << "Renderer initialized: " << w << "x" << h << std::endl;
}

void Renderer::renderFirstPerson(sf::RenderWindow& window,
                                 const Player& player,
                                 const Maze& maze) {
    // 清空屏幕（天空和地板）
    window.clear(sf::Color(50, 50, 50));  // 深灰色背景

    // 绘制天空（上半部分）
    sf::RectangleShape ceiling({static_cast<float>(screenWidth),
                                static_cast<float>(screenHeight / 2)});
    ceiling.setFillColor(sf::Color(100, 100, 150));  // 蓝灰色天空
    ceiling.setPosition({0, 0});
    window.draw(ceiling);

    // 绘制地板（下半部分）
    sf::RectangleShape floor({static_cast<float>(screenWidth),
                             static_cast<float>(screenHeight / 2)});
    floor.setFillColor(sf::Color(80, 80, 80));  // 深灰色地板
    floor.setPosition({0, static_cast<float>(screenHeight / 2)});
    window.draw(floor);

    // 核心：投射光线并绘制墙壁
    castRays(window, player, maze);
}

void Renderer::castRays(sf::RenderWindow& window,
                       const Player& player,
                       const Maze& maze) {
    // 获取玩家信息
    float posX = player.getX();
    float posY = player.getY();
    float dirX = player.getDirX();
    float dirY = player.getDirY();
    float planeX = player.getPlaneX();
    float planeY = player.getPlaneY();

    // 对屏幕的每一列投射一条光线
    for (int x = 0; x < screenWidth; x++) {
        // 计算光线方向
        // cameraX 在 [-1, 1] 范围内，表示当前列在屏幕上的位置
        float cameraX = 2.0f * x / static_cast<float>(screenWidth) - 1.0f;

        // 光线方向 = 玩家方向 + 相机平面 × cameraX
        float rayDirX = dirX + planeX * cameraX;
        float rayDirY = dirY + planeY * cameraX;

        // 玩家所在的地图格子
        int mapX = static_cast<int>(posX);
        int mapY = static_cast<int>(posY);

        // 到下一个格子边界的距离
        float deltaDistX = (rayDirX == 0) ? 1e30f : std::abs(1.0f / rayDirX);
        float deltaDistY = (rayDirY == 0) ? 1e30f : std::abs(1.0f / rayDirY);

        // 光线前进的方向（-1或+1）
        int stepX, stepY;
        // 从当前位置到第一个边界的距离
        float sideDistX, sideDistY;

        // 计算步进方向和初始边界距离
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

        // DDA算法：沿着光线前进，直到碰到墙
        bool hit = false;  // 是否碰到墙
        int side = 0;      // 碰到的是哪个方向的墙（0=东西，1=南北）

        while (!hit) {
            // 前进到下一个格子
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

        // 计算距离（避免鱼眼效果！）
        float perpWallDist;
        if (side == 0) {
            perpWallDist = (mapX - posX + (1 - stepX) / 2) / rayDirX;
        } else {
            perpWallDist = (mapY - posY + (1 - stepY) / 2) / rayDirY;
        }

        // 计算墙的高度（距离越近，墙越高）
        int lineHeight = static_cast<int>(screenHeight / perpWallDist);

        // 计算墙在屏幕上的起始和结束Y坐标
        int drawStart = -lineHeight / 2 + screenHeight / 2;
        if (drawStart < 0) drawStart = 0;

        int drawEnd = lineHeight / 2 + screenHeight / 2;
        if (drawEnd >= screenHeight) drawEnd = screenHeight - 1;

        // 根据墙的朝向选择颜色
        sf::Color wallColor = getWallColor(side);

        // 在屏幕上绘制这一列
        sf::RectangleShape wallStripe({1.0f, static_cast<float>(drawEnd - drawStart)});
        wallStripe.setPosition({static_cast<float>(x), static_cast<float>(drawStart)});
        wallStripe.setFillColor(wallColor);
        window.draw(wallStripe);
    }
}

sf::Color Renderer::getWallColor(int side) {
    // side = 0: 东西方向的墙（垂直墙）
    // side = 1: 南北方向的墙（水平墙）

    if (side == 0) {
        return sf::Color(200, 200, 200);  // 浅灰色
    } else {
        return sf::Color(150, 150, 150);  // 深灰色（制造阴影效果）
    }
}

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
```

---

### **步骤3：集成到Game类**

修改 `Game.h`，添加Renderer：

```cpp
#include "Renderer.h"

class Game {
    // ... 其他成员 ...
    Renderer renderer;  // 添加渲染器

public:
    Game();  // 需要初始化renderer
};
```

修改 `Game.cpp` 构造函数：

```cpp
Game::Game()
    : window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Horror Maze")
    , gameState(GameState::Menu)
    , viewMode(ViewMode::TopDown)
    , score(0)
    , lives(3)
    , currentLevel(1)
    , renderer(WINDOW_WIDTH, WINDOW_HEIGHT)  // 初始化渲染器
{
    // ... 其他初始化代码 ...
}
```

修改 `Game::render()` 函数：

```cpp
void Game::render() {
    window.clear(sf::Color::Black);

    if (gameState == GameState::Menu) {
        // ... 菜单渲染 ...
    } else if (gameState == GameState::Playing) {
        if (viewMode == ViewMode::TopDown) {
            // 俯视图
            renderer.renderTopDown(window, player, maze);
        } else {
            // 第一人称（3D）
            renderer.renderFirstPerson(window, player, maze);
        }

        // HUD
        // ... HUD渲染 ...
    }

    window.display();
}
```

---

## 🎮 测试步骤

### **测试1：编译**
```bash
按 Ctrl + Shift + B 编译
```

### **测试2：运行**
```bash
按 F5 运行游戏
```

### **测试3：切换视角**
1. 按 Enter 开始游戏
2. 看到俯视图
3. **按 Tab 键** → 切换到第一人称
4. 你应该能看到：
   - 上半部分：蓝灰色天空
   - 中间：灰色3D墙壁
   - 下半部分：深灰色地板

### **测试4：移动测试**
- W：前进（墙壁变近）
- S：后退（墙壁变远）
- A：左转（视角旋转）
- D：右转（视角旋转）

### **测试5：性能检查**
- 游戏应该流畅运行（60 FPS）
- 如果卡顿，降低窗口分辨率

---

## 🐛 常见问题

### **问题1：看到的图像扭曲（鱼眼效果）**

**原因**：没有使用垂直距离（perpWallDist）

**解决**：
确保使用的是 `perpWallDist`，而不是真实距离！

```cpp
// ❌ 错误：会产生鱼眼效果
float wallDist = sqrt(dx*dx + dy*dy);

// ✅ 正确：垂直距离
float perpWallDist = (mapX - posX + ...) / rayDirX;
```

### **问题2：墙壁有缝隙**

**原因**：浮点数精度问题

**解决**：
墙条的宽度设为1.0f，确保相邻的条完全连接。

### **问题3：性能低（卡顿）**

**解决**：
- 降低窗口分辨率（改为800×600）
- 减少光线数量（每2列发射一条光线）

### **问题4：看不到墙**

**检查**：
1. 玩家初始位置是否在迷宫内？
2. 玩家方向向量是否正确？
3. DDA算法是否正确检测墙？

---

## 📊 预期时间分配

| 任务 | 预计时间 | 说明 |
|------|----------|------|
| 阅读Lode教程 | 30分钟 | 理解原理很重要 |
| 创建Renderer类 | 1小时 | 基础框架 |
| 实现DDA算法 | 2小时 | 核心难点 |
| 调试和优化 | 1.5小时 | 修复bug和效果 |
| 测试 | 30分钟 | 全面测试 |
| **总计** | **5.5小时** | 独立完成 |

---

## 💡 开发建议

### **1. 先理解再写代码**
- 花30分钟读Lode教程
- 理解DDA算法原理
- 画图理解光线如何前进

### **2. 逐步实现**
1. 先让第一人称能显示（即使全黑也行）
2. 再添加天空和地板
3. 最后添加墙壁渲染
4. 一步步调试

### **3. 使用调试输出**
```cpp
std::cout << "Ray " << x << ": hit at (" << mapX << "," << mapY
          << "), dist=" << perpWallDist << std::endl;
```

### **4. 参考现成代码**
- Lode教程有完整代码
- 可以直接参考并理解
- 关键是理解每一行的作用

### **5. 不要追求完美**
- 第一版只要能看到墙就行
- 颜色、纹理、光照可以Day 3再优化
- 先让游戏能玩！

---

## 🎯 Day 2 成功标准

完成以下所有项，Day 2就成功了：

- [ ] 能按Tab键切换俯视图/第一人称
- [ ] 第一人称中能看到3D墙壁
- [ ] 墙的高度随距离变化（近高远低）
- [ ] 没有鱼眼效果
- [ ] WASD能正常移动和转向
- [ ] 游戏流畅（不卡顿）
- [ ] 能走到出口并胜利

---

## 📚 额外学习资源

### **视频教程**
- YouTube搜索："Raycasting Tutorial"
- 推荐：3DSage的Raycasting系列

### **在线演示**
- https://lodev.org/cgtutor/raycasting.html（有交互演示）

### **开源项目**
- GitHub搜索："raycasting C++"
- 可以参考但要理解

---

## 🚀 开始吧！

1. **先读Lode教程**（30分钟）
2. **创建Renderer类**（按照上面的代码）
3. **编译测试**
4. **逐步调试**

记住：
- ✅ 可以参考现成代码
- ✅ 可以使用AI辅助
- ✅ 但要理解每一行！

**祝你Day 2开发顺利！** 🎮✨

有任何问题随时问我！

---

**最后更新**：2024-12-21
**当前状态**：准备开始 Day 2 ⏰

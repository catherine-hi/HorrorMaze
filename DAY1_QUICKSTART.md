# 🚀 快速开始指南 - 第1天任务

## 📋 今天的目标

**第1天（Day 1）**：建立基础框架，让玩家能在迷宫中移动

---

## 👥 任务分配

### 人员A：迷宫系统 + 俯视图渲染

**预计时间**：6-8小时

**任务清单**：
- [ ] 创建 `Maze.h` 和 `Maze.cpp`
- [ ] 实现地图数据结构
- [ ] 实现从文本文件加载地图
- [ ] 手工设计第一关地图（20×20）
- [ ] 实现俯视图渲染（简单方块）

### 人员B：玩家系统 + 移动控制

**预计时间**：6-8小时

**任务清单**：
- [ ] 创建 `Player.h` 和 `Player.cpp`
- [ ] 实现玩家位置和方向属性
- [ ] 实现键盘输入监听（WASD）
- [ ] 实现基础移动逻辑
- [ ] 实现碰撞检测（调用Maze）

---

## 💻 人员A：详细步骤

### 步骤1：创建Maze类（30分钟）

在 `HorrorMaze` 文件夹中创建 `Maze.h`：

```cpp
#pragma once
#include <vector>
#include <string>
#include <SFML/Graphics.hpp>

class Maze {
public:
    Maze();

    // 加载地图
    bool loadFromFile(const std::string& filename);

    // 地图查询
    bool isWall(int x, int y) const;
    int getCell(int x, int y) const;
    void setCell(int x, int y, int value);

    // 获取地图信息
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    sf::Vector2i getPlayerStart() const { return playerStart; }
    sf::Vector2i getExitPos() const { return exitPos; }

    // 渲染（俯视图）
    void renderTopDown(sf::RenderWindow& window, float cellSize) const;

private:
    int width;
    int height;
    std::vector<std::vector<int>> map;  // 0=空地 1=墙 2=出口 3=双胞胎 4=雪墙 5=打火机
    sf::Vector2i playerStart;
    sf::Vector2i exitPos;
};
```

创建 `Maze.cpp`：

```cpp
#include "Maze.h"
#include <fstream>
#include <iostream>

Maze::Maze() : width(0), height(0) {
}

bool Maze::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法打开地图文件: " << filename << std::endl;
        return false;
    }

    // 读取宽度和高度
    file >> width >> height;

    // 初始化地图
    map.resize(height, std::vector<int>(width, 0));

    // 读取地图数据
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            file >> map[y][x];

            // 记录特殊位置
            if (map[y][x] == 2) {  // 出口
                exitPos = sf::Vector2i(x, y);
            }
        }
    }

    // 默认起点（左上角第一个空地）
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (map[y][x] == 0) {
                playerStart = sf::Vector2i(x, y);
                file.close();
                return true;
            }
        }
    }

    file.close();
    return true;
}

bool Maze::isWall(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return true;  // 边界视为墙
    }
    return map[y][x] == 1;
}

int Maze::getCell(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 1;  // 边界
    }
    return map[y][x];
}

void Maze::setCell(int x, int y, int value) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        map[y][x] = value;
    }
}

void Maze::renderTopDown(sf::RenderWindow& window, float cellSize) const {
    sf::RectangleShape cell(sf::Vector2f(cellSize, cellSize));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            cell.setPosition(x * cellSize, y * cellSize);

            // 根据类型设置颜色
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

            // 绘制网格线
            cell.setFillColor(sf::Color::Transparent);
            cell.setOutlineColor(sf::Color(100, 100, 100));
            cell.setOutlineThickness(1.0f);
            window.draw(cell);
        }
    }
}
```

### 步骤2：创建地图文件（30分钟）

在项目根目录创建 `assets/maps/` 文件夹，然后创建 `level1.txt`：

```
20 20
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 1
1 0 1 1 1 1 1 0 1 0 1 1 1 1 1 1 1 1 0 1
1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1
1 0 1 0 1 1 1 1 1 1 1 1 1 1 1 1 0 1 0 1
1 0 0 0 1 0 0 0 0 0 0 0 0 0 0 1 0 1 0 1
1 1 1 0 1 0 1 1 1 1 1 1 1 1 0 1 0 1 0 1
1 0 0 0 1 0 1 0 0 0 0 0 0 1 0 0 0 1 0 1
1 0 1 1 1 0 1 0 1 1 1 1 0 1 1 1 1 1 0 1
1 0 0 0 0 0 1 0 0 0 0 1 0 0 0 0 0 0 0 1
1 1 1 1 1 0 1 1 1 1 0 1 0 1 1 1 1 1 0 1
1 0 0 0 1 0 0 0 0 0 0 1 0 1 3 0 0 1 0 1
1 0 1 0 1 1 1 1 1 1 1 1 0 1 0 1 0 1 0 1
1 0 1 0 0 0 0 0 0 0 0 0 0 1 0 1 0 4 0 1
1 0 1 1 1 1 1 1 1 1 1 1 1 1 0 1 1 1 0 1
1 0 0 0 0 0 0 0 5 0 0 0 0 0 0 0 0 0 0 1
1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 1
1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1
1 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 2 1
1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
```

**地图说明**：
- 0 = 空地
- 1 = 墙
- 2 = 出口（右下角）
- 3 = 双胞胎位置
- 4 = 雪墙（可躲藏）
- 5 = 打火机道具

### 步骤3：修改Game类集成迷宫（1小时）

在 `Game.h` 中添加：

```cpp
#include "Maze.h"

class Game {
    // ... 现有成员 ...
    Maze maze;
    bool showTopDown;  // 是否显示俯视图（调试用）
};
```

在 `Game.cpp` 构造函数中：

```cpp
Game::Game()
    : window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Horror Maze")
    , gameState(GameState::Menu)
    , viewMode(ViewMode::TopDown)
    , score(0)
    , lives(3)
    , currentLevel(1)
    , showTopDown(true)  // 第一天先用俯视图
{
    window.setFramerateLimit(static_cast<unsigned int>(TARGET_FPS));

    // 加载地图
    if (!maze.loadFromFile("assets/maps/level1.txt")) {
        std::cerr << "地图加载失败！" << std::endl;
    }
}
```

在 `Game::render()` 中添加迷宫渲染：

```cpp
void Game::render() {
    window.clear(sf::Color::Black);

    if (gameState == GameState::Menu) {
        // ... 菜单绘制 ...
    } else if (gameState == GameState::Playing) {
        if (showTopDown) {
            // 俯视图
            float cellSize = std::min(
                WINDOW_WIDTH / (float)maze.getWidth(),
                WINDOW_HEIGHT / (float)maze.getHeight()
            );
            maze.renderTopDown(window, cellSize);
        }
        // ... HUD绘制 ...
    }

    window.display();
}
```

### 步骤4：测试（30分钟）

编译并运行，确认：
- [ ] 程序能正常启动
- [ ] 按Enter进入游戏
- [ ] 能看到迷宫（俯视图）
- [ ] 墙、空地、出口颜色正确

---

## 💻 人员B：详细步骤

### 步骤1：创建Player类（30分钟）

创建 `Player.h`：

```cpp
#pragma once
#include <SFML/Graphics.hpp>

class Maze;  // 前向声明

class Player {
public:
    Player();
    Player(float startX, float startY);

    // 移动和旋转
    void move(float deltaTime, const Maze& maze);
    void rotate(float angle);

    // 更新
    void update(float deltaTime, const Maze& maze);

    // 获取信息
    float getX() const { return x; }
    float getY() const { return y; }
    float getDirX() const { return dirX; }
    float getDirY() const { return dirY; }

    // 渲染（俯视图）
    void renderTopDown(sf::RenderWindow& window, float cellSize) const;

private:
    // 位置
    float x, y;

    // 方向向量
    float dirX, dirY;

    // 相机平面向量（用于3D渲染）
    float planeX, planeY;

    // 移动参数
    float moveSpeed;
    float rotateSpeed;

    // 输入状态
    bool movingForward;
    bool movingBackward;
    bool rotatingLeft;
    bool rotatingRight;

    void handleInput();
    bool checkCollision(float newX, float newY, const Maze& maze) const;
};
```

创建 `Player.cpp`：

```cpp
#include "Player.h"
#include "Maze.h"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>

Player::Player() : Player(1.5f, 1.5f) {
}

Player::Player(float startX, float startY)
    : x(startX)
    , y(startY)
    , dirX(-1.0f), dirY(0.0f)      // 初始朝左
    , planeX(0.0f), planeY(0.66f)   // 相机平面
    , moveSpeed(3.0f)                // 3格/秒
    , rotateSpeed(3.0f)              // 3弧度/秒
    , movingForward(false)
    , movingBackward(false)
    , rotatingLeft(false)
    , rotatingRight(false)
{
}

void Player::handleInput() {
    movingForward = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
                   sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
    movingBackward = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
    rotatingLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
    rotatingRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
                   sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);
}

bool Player::checkCollision(float newX, float newY, const Maze& maze) const {
    // 检查新位置是否在墙内（使用小圆碰撞体积）
    const float collisionRadius = 0.2f;

    // 检查四个角点
    if (maze.isWall(int(newX - collisionRadius), int(newY - collisionRadius))) return true;
    if (maze.isWall(int(newX + collisionRadius), int(newY - collisionRadius))) return true;
    if (maze.isWall(int(newX - collisionRadius), int(newY + collisionRadius))) return true;
    if (maze.isWall(int(newX + collisionRadius), int(newY + collisionRadius))) return true;

    return false;
}

void Player::move(float deltaTime, const Maze& maze) {
    float moveStep = moveSpeed * deltaTime;

    if (movingForward) {
        float newX = x + dirX * moveStep;
        float newY = y + dirY * moveStep;

        if (!checkCollision(newX, y, maze)) x = newX;
        if (!checkCollision(x, newY, maze)) y = newY;
    }

    if (movingBackward) {
        float newX = x - dirX * moveStep;
        float newY = y - dirY * moveStep;

        if (!checkCollision(newX, y, maze)) x = newX;
        if (!checkCollision(x, newY, maze)) y = newY;
    }
}

void Player::rotate(float angle) {
    // 旋转方向向量
    float oldDirX = dirX;
    dirX = dirX * cos(angle) - dirY * sin(angle);
    dirY = oldDirX * sin(angle) + dirY * cos(angle);

    // 旋转相机平面
    float oldPlaneX = planeX;
    planeX = planeX * cos(angle) - planeY * sin(angle);
    planeY = oldPlaneX * sin(angle) + planeY * cos(angle);
}

void Player::update(float deltaTime, const Maze& maze) {
    handleInput();

    // 旋转
    if (rotatingLeft) {
        rotate(rotateSpeed * deltaTime);
    }
    if (rotatingRight) {
        rotate(-rotateSpeed * deltaTime);
    }

    // 移动
    move(deltaTime, maze);
}

void Player::renderTopDown(sf::RenderWindow& window, float cellSize) const {
    // 绘制玩家（小圆点）
    sf::CircleShape playerCircle(cellSize * 0.3f);
    playerCircle.setFillColor(sf::Color::Red);
    playerCircle.setOrigin(cellSize * 0.3f, cellSize * 0.3f);
    playerCircle.setPosition(x * cellSize, y * cellSize);
    window.draw(playerCircle);

    // 绘制朝向（线条）
    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(x * cellSize, y * cellSize), sf::Color::Yellow),
        sf::Vertex(sf::Vector2f((x + dirX * 0.5f) * cellSize,
                                (y + dirY * 0.5f) * cellSize), sf::Color::Yellow)
    };
    window.draw(line, 2, sf::Lines);
}
```

### 步骤2：集成到Game类（1小时）

在 `Game.h` 中添加：

```cpp
#include "Player.h"

class Game {
    // ... 现有成员 ...
    Player player;
};
```

在 `Game.cpp` 中修改：

```cpp
// 构造函数
Game::Game()
    : // ... 其他初始化 ...
{
    window.setFramerateLimit(static_cast<unsigned int>(TARGET_FPS));

    // 加载地图
    if (!maze.loadFromFile("assets/maps/level1.txt")) {
        std::cerr << "地图加载失败！" << std::endl;
    }

    // 初始化玩家位置
    sf::Vector2i startPos = maze.getPlayerStart();
    player = Player(startPos.x + 0.5f, startPos.y + 0.5f);
}

// update函数
void Game::update(float deltaTime) {
    if (gameState == GameState::Playing) {
        player.update(deltaTime, maze);

        // TODO: 检查胜利条件
        // if (player到达出口) { gameState = GameState::Victory; }
    }
}

// render函数
void Game::render() {
    window.clear(sf::Color::Black);

    if (gameState == GameState::Menu) {
        // ... 菜单绘制 ...
    } else if (gameState == GameState::Playing) {
        if (showTopDown) {
            float cellSize = std::min(
                WINDOW_WIDTH / (float)maze.getWidth(),
                WINDOW_HEIGHT / (float)maze.getHeight()
            );

            maze.renderTopDown(window, cellSize);
            player.renderTopDown(window, cellSize);
        }
        // ... HUD绘制 ...
    }

    window.display();
}
```

### 步骤3：测试（30分钟）

编译并运行，确认：
- [ ] 玩家显示在起点位置（红色圆点）
- [ ] WASD键能控制移动
- [ ] 上下键能控制移动
- [ ] AD键能旋转（看到黄色方向线旋转）
- [ ] **不能穿过墙壁**（最重要）

---

## 🤝 晚上联调（30分钟）

### 联调步骤

1. **合并代码**：
   - 人员A把 `Maze.h/cpp` 提交
   - 人员B把 `Player.h/cpp` 提交
   - 合并到同一个项目

2. **测试清单**：
   ```
   □ 程序能编译通过
   □ 迷宫正确显示
   □ 玩家在起点显示
   □ 玩家能移动（WASD）
   □ 玩家能旋转（AD）
   □ 碰撞检测正确（不能穿墙）
   □ 帧率正常（不卡顿）
   ```

3. **调整参数**：
   - 玩家移动速度是否合适？
   - 旋转速度是否合适？
   - 碰撞检测是否太灵敏/不够？

4. **记录问题**：
   - 把发现的Bug记录下来
   - 明天优先修复

---

## ⚠️ 常见问题

### Q1: 编译错误 "无法打开文件 Maze.h"
**解决**：确保 `Maze.h` 和 `Maze.cpp` 都已添加到Visual Studio项目中。
- 右键项目 → 添加 → 现有项

### Q2: 运行时找不到地图文件
**解决**：
- 确保 `assets/maps/level1.txt` 存在
- 或者使用绝对路径：`"E:/cs106A data structures/Final_Project/HorrorMaze/assets/maps/level1.txt"`

### Q3: 玩家能穿墙
**解决**：
- 检查 `checkCollision` 函数
- 检查 `maze.isWall()` 是否正确返回
- 添加调试输出：
  ```cpp
  std::cout << "Checking (" << newX << ", " << newY << "): "
            << maze.isWall(int(newX), int(newY)) << std::endl;
  ```

### Q4: 窗口一片黑
**解决**：
- 检查 `render()` 函数是否被调用
- 检查地图是否正确加载
- 检查 `cellSize` 计算是否正确

---

## ✅ 第1天成功标准

今天结束时，你们应该能：

1. ✅ 编译并运行程序
2. ✅ 看到迷宫（俯视图）
3. ✅ 玩家能用WASD移动
4. ✅ 玩家不能穿墙
5. ✅ 玩家能旋转方向

**如果以上都实现了，第1天就成功了！** 🎉

---

## 📝 明天预告（第2天）

**人员A**：
- 开始实现3D渲染（光线投射）
- 参考Lode教程

**人员B**：
- 实现3种移动模式（走/跑/蹲）
- 优化移动手感

---

祝第一天开发顺利！有问题随时沟通。💪

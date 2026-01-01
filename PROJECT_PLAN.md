# 🎮 Horror Maze - 项目开发规划

## 📋 项目概述
**项目名称**：Horror Maze（恐怖迷宫）
**开发周期**：9天
**团队规模**：2人
**技术栈**：C++ + SFML 3.0
**3D技术**：光线投射（Raycasting）伪3D

---

## 🎯 核心需求清单

### ✅ 必需功能
1. ✓ 键盘控制（上下左右）
2. ✓ 碰撞检测（不能穿墙）
3. ✓ 图形界面（主菜单：开始游戏、退出游戏）
4. ✓ 重新开始功能
5. ✓ HUD信息显示（生命、关卡、分数）
6. ✓ 胜利/失败条件
7. ✓ 至少3种道具或陷阱
8. ✓ 追踪型敌人（A*算法）
9. ✓ 游戏时长1-3分钟/关卡

---

## 🏗️ 模块划分

### 模块1：迷宫系统
**文件**：`Maze.h`, `Maze.cpp`
**优先级**：⭐⭐⭐⭐⭐ 核心
**负责人**：人员A
**时间**：第1-2天

**功能清单**：
```cpp
class Maze {
    // 迷宫数据（0=空地，1=墙，2=出口，3+=道具）
    std::vector<std::vector<int>> map;

    void generateMaze();        // 迷宫生成（DFS算法）
    bool isWall(int x, int y);  // 检查是否是墙
    int getCell(int x, int y);  // 获取单元格类型
    void setCell(int x, int y, int value);
};
```

**实现要点**：
- 迷宫大小：15x15 或 20x20
- 使用DFS递归回溯生成
- 确保有唯一路径到出口
- 预留道具和陷阱位置

---

### 模块2：玩家系统
**文件**：`Player.h`, `Player.cpp`
**优先级**：⭐⭐⭐⭐⭐ 核心
**负责人**：人员B
**时间**：第1-2天

**功能清单**：
```cpp
class Player {
    float x, y;           // 位置
    float dirX, dirY;     // 方向向量
    float speed;          // 移动速度
    int lives;            // 生命值
    int score;            // 分数

    void move(float deltaTime, const Maze& maze);
    void rotate(float angle);
    void takeDamage();
    void addScore(int points);
    bool isAlive();
};
```

**实现要点**：
- 监听上下左右键
- 每帧根据速度和方向更新位置
- 移动前检查碰撞（调用Maze::isWall）
- 旋转视角（第一人称用）

---

### 模块3：3D渲染引擎
**文件**：`Renderer.h`, `Renderer.cpp`
**优先级**：⭐⭐⭐⭐ 重要
**负责人**：人员A
**时间**：第3-4天

**功能清单**：
```cpp
class Renderer {
    void renderFirstPerson(sf::RenderWindow& window,
                          const Player& player,
                          const Maze& maze);
    void renderTopDown(sf::RenderWindow& window,
                      const Player& player,
                      const Maze& maze);
    void renderMiniMap(sf::RenderWindow& window,
                       const Player& player,
                       const Maze& maze);
};
```

**光线投射算法步骤**：
1. 对屏幕每一列发射一条射线
2. 计算射线与墙的交点距离
3. 根据距离计算墙的高度
4. 绘制垂直条带（距离越近墙越高）

**参考资源**：
- Lode's Raycasting Tutorial: https://lodev.org/cgtutor/raycasting.html
- 可以先实现俯视图，再实现第一人称

---

### 模块4：敌人AI系统
**文件**：`Enemy.h`, `Enemy.cpp`, `Pathfinding.h`, `Pathfinding.cpp`
**优先级**：⭐⭐⭐⭐ 重要
**负责人**：人员B
**时间**：第3-4天

**功能清单**：
```cpp
class Enemy {
    float x, y;
    float speed;
    std::vector<sf::Vector2i> path;  // A*路径

    void update(float deltaTime, const Player& player, const Maze& maze);
    void findPath(const Player& player, const Maze& maze);  // A*寻路
    bool catchPlayer(const Player& player);  // 检测是否抓到玩家
};

namespace Pathfinding {
    std::vector<sf::Vector2i> aStar(sf::Vector2i start,
                                     sf::Vector2i goal,
                                     const Maze& maze);
}
```

**A*算法要点**：
- 开放列表（待探索节点）
- 关闭列表（已探索节点）
- f(n) = g(n) + h(n)
  - g(n)：起点到当前节点距离
  - h(n)：当前节点到目标估计距离（曼哈顿距离）
- 每0.5秒重新计算路径

---

### 模块5：道具与陷阱系统
**文件**：`Item.h`, `Item.cpp`, `Trap.h`, `Trap.cpp`
**优先级**：⭐⭐⭐ 中等
**负责人**：人员A
**时间**：第5-6天

**道具类型**：
1. **传送门（Teleporter）**
   ```cpp
   struct Teleporter {
       sf::Vector2i entrance;
       sf::Vector2i exit;
   };
   ```

2. **钥匙与门（Key & Door）**
   ```cpp
   struct Key {
       sf::Vector2i position;
       int colorType;  // 0=红, 1=蓝, 2=绿
   };

   struct Door {
       sf::Vector2i position;
       int colorType;
       bool isOpen;
   };
   ```

3. **能量豆（Power-up）**
   ```cpp
   struct PowerUp {
       sf::Vector2i position;
       enum Type { HEALTH, SCORE, SPEED } type;
       int value;
   };
   ```

**陷阱类型**：
1. **单向通道**：只能从一个方向通过
2. **隐形墙**：定时出现/消失的墙
3. **移动地板**：缓慢移动玩家位置

---

### 模块6：UI系统
**文件**：`UI.h`, `UI.cpp`
**优先级**：⭐⭐⭐ 中等
**负责人**：人员B
**时间**：第5-6天

**界面清单**：
```cpp
class UI {
    sf::Font font;

    void drawMainMenu(sf::RenderWindow& window);
    void drawHUD(sf::RenderWindow& window, const Player& player, int level);
    void drawGameOver(sf::RenderWindow& window, bool victory, int score);
    void drawPauseMenu(sf::RenderWindow& window);
};
```

**HUD元素**：
- 生命值：❤❤❤
- 分数：Score: 1500
- 关卡：Level: 1
- 钥匙收集状态：🔑 2/3
- 小地图（可选）

**字体资源**：
- 下载免费字体（如 Arial.ttf 或恐怖风格字体）
- 放在 `assets/fonts/` 目录

---

### 模块7：游戏逻辑管理
**文件**：`Game.h`, `Game.cpp`（已存在，需扩展）
**优先级**：⭐⭐⭐⭐⭐ 核心
**负责人**：两人协作
**时间**：第7天

**需要添加的功能**：
```cpp
class Game {
    // 已有成员...
    Maze maze;
    Player player;
    std::vector<Enemy> enemies;
    std::vector<PowerUp> powerUps;
    std::vector<Door> doors;
    Renderer renderer;
    UI ui;

    void initLevel(int level);       // 初始化关卡
    void checkCollisions();          // 检测碰撞
    void checkVictoryCondition();    // 检查胜利
    void restartGame();              // 重新开始
};
```

**胜利条件（选一种）**：
- 收集所有钥匙后到达出口
- 生存2分钟
- 达到指定分数

**失败条件**：
- 生命值归零

---

### 模块8：资源管理
**文件**：`ResourceManager.h`, `ResourceManager.cpp`
**优先级**：⭐⭐ 低
**负责人**：人员B
**时间**：第6天

**功能**：
```cpp
class ResourceManager {
    static std::map<std::string, sf::Font> fonts;

    static sf::Font& getFont(const std::string& name);
    static void loadResources();
};
```

---

## 📅 详细时间安排

### **第1天**（2024-12-XX）
**人员A**：
- [ ] 创建 `Maze.h/cpp` 文件
- [ ] 实现基本数据结构（vector<vector<int>>）
- [ ] 实现简单迷宫（先用固定地图测试）
- [ ] 实现碰撞检测函数

**人员B**：
- [ ] 创建 `Player.h/cpp` 文件
- [ ] 实现玩家位置和方向
- [ ] 实现键盘输入监听
- [ ] 实现基本移动逻辑（不考虑碰撞）

**晚上联调**：
- [ ] 玩家在固定迷宫中移动
- [ ] 俯视图渲染（简单方块）

---

### **第2天**（2024-12-XX）
**人员A**：
- [ ] 实现迷宫生成算法（DFS）
- [ ] 测试生成的迷宫是否可达
- [ ] 添加出口位置

**人员B**：
- [ ] 集成碰撞检测到移动逻辑
- [ ] 调整移动速度和流畅度
- [ ] 添加旋转功能（为第一人称准备）

**晚上联调**：
- [ ] 玩家在随机迷宫中移动
- [ ] 无法穿墙
- [ ] 能到达出口（简单触发）

---

### **第3天**（2024-12-XX）
**人员A**：
- [ ] 创建 `Renderer.h/cpp`
- [ ] 实现光线投射算法
- [ ] 渲染基本的第一人称墙体

**人员B**：
- [ ] 创建 `Enemy.h/cpp` 和 `Pathfinding.h/cpp`
- [ ] 实现A*算法骨架
- [ ] 测试A*寻路（打印路径）

**晚上联调**：
- [ ] 第一人称视角能看到迷宫
- [ ] 敌人能计算路径（暂不移动）

---

### **第4天**（2024-12-XX）
**人员A**：
- [ ] 优化渲染性能
- [ ] 添加墙体颜色/纹理
- [ ] 实现小地图功能

**人员B**：
- [ ] 实现敌人移动逻辑
- [ ] 敌人追踪玩家
- [ ] 碰撞检测（敌人抓到玩家）

**晚上联调**：
- [ ] 敌人能追踪并抓到玩家
- [ ] 第一人称视角能看到敌人（可选）

---

### **第5天**（2024-12-XX）
**人员A**：
- [ ] 创建 `Item.h/cpp`
- [ ] 实现传送门系统
- [ ] 实现钥匙和门系统
- [ ] 实现能量豆

**人员B**：
- [ ] 创建 `UI.h/cpp`
- [ ] 实现主菜单界面
- [ ] 加载字体资源
- [ ] 实现HUD显示

**晚上联调**：
- [ ] 道具能拾取
- [ ] UI正常显示

---

### **第6天**（2024-12-XX）
**人员A**：
- [ ] 创建 `Trap.h/cpp`
- [ ] 实现单向通道
- [ ] 实现隐形墙
- [ ] 实现移动地板

**人员B**：
- [ ] 完善游戏结束界面
- [ ] 实现重新开始功能
- [ ] 创建 ResourceManager

**晚上联调**：
- [ ] 陷阱系统工作正常
- [ ] 游戏流程完整

---

### **第7天**（2024-12-XX）
**两人协作**：
- [ ] 整合所有模块到 Game.cpp
- [ ] 实现胜利/失败条件
- [ ] 关卡系统（如果时间允许）
- [ ] 修复所有编译错误
- [ ] 基础Bug修复

**目标**：完整游戏可玩

---

### **第8天**（2024-12-XX）
**人员A**：
- [ ] 性能优化（如果有卡顿）
- [ ] 调整游戏难度
- [ ] 恐怖氛围营造（暗色调、闪烁效果）

**人员B**：
- [ ] 全面测试游戏
- [ ] 记录所有Bug
- [ ] 修复崩溃和严重Bug

**晚上联调**：
- [ ] 完整测试流程（菜单→游戏→胜利/失败→重新开始）

---

### **第9天**（2024-12-XX）
**人员A**：
- [ ] 撰写 README.md
  - 项目结构
  - 编译步骤
  - 运行方法
  - 游戏玩法
  - 技术实现

**人员B**：
- [ ] 最后一轮Bug修复
- [ ] 确保程序稳定运行
- [ ] 准备演示视频/截图

**下午**：
- [ ] 最终测试
- [ ] 准备答辩材料

---

## 🚨 风险管理

### 高风险项
1. **光线投射算法复杂**
   - 缓解：先实现俯视图，3D作为加分项
   - 参考现成代码：Lode's tutorial

2. **A*算法难调试**
   - 缓解：使用在线可视化工具测试
   - 参考现成C++实现

3. **时间不足**
   - 缓解：优先实现核心功能
   - 道具数量可以减少到3个最简单的

### 备用方案
- 如果3D实现困难，使用俯视图（2D）
- 如果A*太难，使用简单追踪（直线移动）
- 关卡数量可以只做1个

---

## 📚 参考资源

### 光线投射（Raycasting）
- Lode's Raycasting Tutorial: https://lodev.org/cgtutor/raycasting.html
- YouTube: "Making a raycasting engine in C++"

### A*算法
- Red Blob Games: https://www.redblobgames.com/pathfinding/a-star/
- Wikipedia A* algorithm

### SFML教程
- 官方文档: https://www.sfml-dev.org/learn.php
- SFML游戏开发书籍

### 开源参考
- GitHub搜索: "maze game C++ SFML"
- GitHub搜索: "raycasting engine C++"

---

## ✅ 每日检查清单

**每天下班前**：
- [ ] 代码提交到Git（如果使用版本控制）
- [ ] 更新进度文档
- [ ] 明天工作计划
- [ ] 遇到的问题记录

**每天开始前**：
- [ ] 回顾昨天进度
- [ ] 确认今天目标
- [ ] 分配任务

---

## 🎯 最终交付物

1. [ ] 可运行的游戏程序（.exe）
2. [ ] 完整源代码
3. [ ] README.md文档
4. [ ] 演示视频/截图
5. [ ] 答辩PPT（如需要）

---

## 💡 成功建议

1. **先让游戏能跑起来**，再考虑美化
2. **每天联调**，不要等到最后集成
3. **优先实现核心功能**（移动、追踪、胜利条件）
4. **大胆使用AI**帮助写算法代码
5. **参考开源项目**，但要理解代码
6. **保持沟通**，遇到问题及时讨论
7. **留出2天缓冲时间**处理意外

---

**最后更新**：2024-12-XX
**当前阶段**：规划阶段 ✅


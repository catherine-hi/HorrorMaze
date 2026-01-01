#include "Ghost.h"
#include "Player.h"
#include "Maze.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <random>
#include <set>

// 静态成员初始化
sf::Texture Ghost::s_spriteTexture;
bool Ghost::s_textureLoaded = false;

/**
 * 构造函数：创建一个鬼
 */
Ghost::Ghost(float startX, float startY)
    : x(startX)
    , y(startY)
    , dirX(0.0f)
    , dirY(1.0f)
    , patrolSpeed(1.8f)      // 巡逻速度：较慢
    , alertSpeed(2.4f)       // 警戒速度：中等
    , chaseSpeed(3.5f)       // 追逐速度：较快
    , currentSpeed(0.0f)
    , currentState(State::Patrol)
    , stateChangeTimer(0.0f)
    , alertTimer(0.0f)
    , movePauseTimer(0.0f)
    , movePaused(false)
    , pathIndex(0)
    , pathUpdateTimer(0.0f)
    , lastKnownPlayerCell(0, 0)
    , noPathWarningTimer(0.0f)  // 初始化警告计时器
{
    std::cout << "Ghost spawned at: (" << x << ", " << y << ")" << std::endl;
    chooseRandomDirection();  // 初始随机方向
}

/**
 * 核心更新函数：每帧调用
 */
void Ghost::update(float deltaTime, const Player& player, const Maze& maze) {
    float prevX = x;
    float prevY = y;

    // 更新警告计时器
    if (noPathWarningTimer > 0.0f) {
        noPathWarningTimer -= deltaTime;
    }

    // === 玩家在墙内时的特殊处理 ===
    // 如果玩家在墙内但开着打火机，光会暴露位置，鬼仍然可以检测
    // 只有在墙内且关闭打火机时，鬼才无法检测
    if (player.isInWall() && !player.isLighterOn()) {
        // 玩家在墙内且关闭打火机，鬼看不到，切换到巡逻状态
        if (currentState != State::Patrol) {
            transitionTo(State::Patrol);
            std::cout << "Ghost: Player phased into wall (lighter off), lost target..." << std::endl;
        }
        updatePatrol(deltaTime, maze);  // 执行巡逻行为

        if (deltaTime > 0.0f) {
            float dx = x - prevX;
            float dy = y - prevY;
            currentSpeed = std::sqrt(dx * dx + dy * dy) / deltaTime;
        } else {
            currentSpeed = 0.0f;
        }
        return;  // 直接返回，不进行声音检测
    }

    // 注意：如果玩家在墙内但打火机开着，会继续执行下面的检测逻辑
    // canSeeLighter() 会检测到打火机光

    // 更新状态切换冷却计时器
    if (stateChangeTimer > 0.0f) {
        stateChangeTimer -= deltaTime;
    }

    // === 视听检测 ===
    bool canSee = canSeePlayer(player, maze);
    bool canSeeLighterGlow = canSeeLighter(player, maze);  // 打火机光照检测
    float soundLevel = calculateSoundLevel(player, maze);
    bool canHear = (soundLevel > HEARING_THRESHOLD);

    if (canSee || canSeeLighterGlow || canHear) {
        lastKnownPlayerCell = {
            static_cast<int>(player.getX()),
            static_cast<int>(player.getY())
        };
        if (currentState != State::Chasing) {
            transitionTo(State::Chasing);
            if (canSeeLighterGlow && !canSee) {
                std::cout << "Ghost: LIGHTER DETECTED! CHASING!" << std::endl;
            } else {
                std::cout << "Ghost: CHASING!" << std::endl;
            }
        }
    } else {
        if (currentState == State::Chasing && stateChangeTimer <= 0.0f) {
            transitionTo(State::Alert);
            std::cout << "Ghost: Lost target, alert..." << std::endl;
        } else if (currentState == State::Alert) {
            alertTimer -= deltaTime;
            if (alertTimer <= 0.0f && stateChangeTimer <= 0.0f) {
                transitionTo(State::Patrol);
                std::cout << "Ghost: Back to patrol." << std::endl;
            }
        }
    }

    // === 根据当前状态执行行为 ===
    switch (currentState) {
        case State::Chasing:
            updateChasing(deltaTime, player, maze);
            break;
        case State::Alert:
            updateAlert(deltaTime, maze);
            break;
        case State::Patrol:
        default:
            updatePatrol(deltaTime, maze);
            break;
    }

    if (deltaTime > 0.0f) {
        float dx = x - prevX;
        float dy = y - prevY;
        currentSpeed = std::sqrt(dx * dx + dy * dy) / deltaTime;
    } else {
        currentSpeed = 0.0f;
    }
}

/**
 * 计算鬼能听到的玩家声音强度
 *
 * 算法：
 * 1. 计算欧几里得距离
 * 2. 使用Bresenham直线算法计算声音路径上穿过的墙数量
 * 3. 应用距离衰减和墙壁衰减
 */
float Ghost::calculateSoundLevel(const Player& player, const Maze& maze) const {
    // === 1. 计算距离 ===
    float dx = player.getX() - x;
    float dy = player.getY() - y;
    float distance = std::sqrt(dx * dx + dy * dy);

    float baseSound = PLAYER_SOUND_WALK;
    switch (player.getMoveMode()) {
        case Player::MoveMode::Run:
            baseSound = PLAYER_SOUND_RUN;
            break;
        case Player::MoveMode::Crouch:
            baseSound = PLAYER_SOUND_CROUCH;
            break;
        case Player::MoveMode::Walk:
        default:
            baseSound = PLAYER_SOUND_WALK;
            break;
    }

    // === 2. 基础距离衰减（空气中） ===
    float soundLevel = baseSound / (1.0f + distance * AIR_ATTENUATION);

    // === 3. 计算声音路径上的墙数量（Bresenham直线算法）===
    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = static_cast<int>(player.getX());
    int y1 = static_cast<int>(player.getY());

    int wallCount = 0;

    // Bresenham直线算法
    int dx_line = std::abs(x1 - x0);
    int dy_line = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx_line - dy_line;

    int currentX = x0;
    int currentY = y0;

    while (true) {
        // 检查当前格子是否是墙
        if (maze.isWall(currentX, currentY)) {
            wallCount++;
        }

        // 到达终点
        if (currentX == x1 && currentY == y1) {
            break;
        }

        // Bresenham步进
        int e2 = 2 * err;
        if (e2 > -dy_line) {
            err -= dy_line;
            currentX += sx;
        }
        if (e2 < dx_line) {
            err += dx_line;
            currentY += sy;
        }
    }

    // === 4. 应用墙壁衰减 ===
    for (int i = 0; i < wallCount; i++) {
        soundLevel *= WALL_ATTENUATION_MULT;  // 每堵墙衰减到30%
    }

    return soundLevel;
}

/**
 * 视线检测：在一定距离内且没有墙阻挡
 */
bool Ghost::canSeePlayer(const Player& player, const Maze& maze) const {
    float dx = player.getX() - x;
    float dy = player.getY() - y;
    float distance = std::sqrt(dx * dx + dy * dy);
    if (distance > VISION_RANGE) {
        return false;
    }

    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = static_cast<int>(player.getX());
    int y1 = static_cast<int>(player.getY());

    int dxl = std::abs(x1 - x0);
    int dyl = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dxl - dyl;

    int currentX = x0;
    int currentY = y0;

    while (true) {
        if (maze.isWall(currentX, currentY)) {
            return false;
        }
        if (currentX == x1 && currentY == y1) {
            break;
        }
        int e2 = 2 * err;
        if (e2 > -dyl) {
            err -= dyl;
            currentX += sx;
        }
        if (e2 < dxl) {
            err += dxl;
            currentY += sy;
        }
    }

    return true;
}

/**
 * 打火机光照检测（特殊视觉检测）
 *
 * 规则：
 * 1. 玩家必须开启打火机
 * 2. 光可以穿透1堵墙
 * 3. 如果中间没有墙，无论多远都能检测到
 */
bool Ghost::canSeeLighter(const Player& player, const Maze& maze) const {
    // 玩家必须开启打火机
    if (!player.isLighterOn()) {
        return false;
    }

    // 计算鬼到玩家的直线路径上的墙数量
    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = static_cast<int>(player.getX());
    int y1 = static_cast<int>(player.getY());

    int wallCount = 0;
    int dxl = std::abs(x1 - x0);
    int dyl = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dxl - dyl;

    int currentX = x0;
    int currentY = y0;

    while (true) {
        // 检查当前格子是否是墙
        if (maze.isWall(currentX, currentY)) {
            wallCount++;
        }

        // 到达终点
        if (currentX == x1 && currentY == y1) {
            break;
        }

        // Bresenham步进
        int e2 = 2 * err;
        if (e2 > -dyl) {
            err -= dyl;
            currentX += sx;
        }
        if (e2 < dxl) {
            err += dxl;
            currentY += sy;
        }
    }

    // 光可以穿透1堵墙，所以墙数<=1时可以检测到
    return (wallCount <= 1);
}

void Ghost::transitionTo(State newState) {
    if (newState == currentState) {
        return;
    }

    currentState = newState;
    stateChangeTimer = STATE_CHANGE_COOLDOWN;
    currentPath.clear();
    pathIndex = 0;
    pathUpdateTimer = 0.0f;
    movePauseTimer = 0.0f;
    movePaused = false;

    if (newState == State::Patrol) {
        chooseRandomDirection();
    } else if (newState == State::Alert) {
        alertTimer = ALERT_DURATION;
    }
}

/**
 * 追踪状态行为：使用A*寻路追踪玩家
 */
void Ghost::updateChasing(float deltaTime, const Player& player, const Maze& maze) {
    // === 定期更新路径（避免每帧计算A*） ===
    pathUpdateTimer += deltaTime;
    if (pathUpdateTimer >= PATH_UPDATE_INTERVAL || currentPath.empty()) {
        pathUpdateTimer = 0.0f;

        // 计算到玩家位置的新路径
        int targetX = static_cast<int>(player.getX());
        int targetY = static_cast<int>(player.getY());
        currentPath = findPath(targetX, targetY, maze);
        pathIndex = 0;

        if (currentPath.empty()) {
            // 找不到路径，但不立即切换状态
            // 让声音系统决定是否切换（通过冷却时间）

            // 只在冷却结束时输出警告（避免刷屏）
            if (noPathWarningTimer <= 0.0f) {
                std::cout << "Ghost: No path found to player (player may be in wall)" << std::endl;
                noPathWarningTimer = NO_PATH_WARNING_INTERVAL;  // 重置计时器
            }

            // 直接朝玩家方向移动（尝试绕过障碍）
            float dx = player.getX() - x;
            float dy = player.getY() - y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 0.1f) {
                move(deltaTime, dx / dist, dy / dist, maze);
            }
            return;
        }
    }

    // === 沿着路径移动 ===
    if (!currentPath.empty() && pathIndex < static_cast<int>(currentPath.size())) {
        // 目标：当前路径点的中心
        float targetX = currentPath[pathIndex].x + 0.5f;
        float targetY = currentPath[pathIndex].y + 0.5f;

        // 计算方向向量
        float dx = targetX - x;
        float dy = targetY - y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 0.1f) {
            // 到达当前路径点，移动到下一个
            pathIndex++;
        } else {
            // 朝目标移动
            float targetDirX = dx / dist;
            float targetDirY = dy / dist;
            move(deltaTime, targetDirX, targetDirY, maze);
        }
    }
}

/**
 * 巡逻状态行为：走走停停随机移动
 */
void Ghost::updatePatrol(float deltaTime, const Maze& maze) {
    movePauseTimer += deltaTime;
    if (movePaused) {
        if (movePauseTimer >= PATROL_PAUSE_DURATION) {
            movePauseTimer = 0.0f;
            movePaused = false;
            chooseRandomDirection();
        }
        return;
    }

    if (movePauseTimer >= PATROL_MOVE_DURATION) {
        movePauseTimer = 0.0f;
        movePaused = true;
        return;
    }

    move(deltaTime, dirX, dirY, maze);
}

/**
 * 警戒状态行为：走走停停靠近最后位置
 */
void Ghost::updateAlert(float deltaTime, const Maze& maze) {
    movePauseTimer += deltaTime;
    if (movePaused) {
        if (movePauseTimer >= ALERT_PAUSE_DURATION) {
            movePauseTimer = 0.0f;
            movePaused = false;
        }
        return;
    }

    if (movePauseTimer >= ALERT_MOVE_DURATION) {
        movePauseTimer = 0.0f;
        movePaused = true;
        return;
    }

    pathUpdateTimer += deltaTime;
    if (pathUpdateTimer >= PATH_UPDATE_INTERVAL || currentPath.empty()) {
        pathUpdateTimer = 0.0f;
        currentPath = findPath(lastKnownPlayerCell.x, lastKnownPlayerCell.y, maze);
        pathIndex = 0;
    }

    if (!currentPath.empty() && pathIndex < static_cast<int>(currentPath.size())) {
        float targetX = currentPath[pathIndex].x + 0.5f;
        float targetY = currentPath[pathIndex].y + 0.5f;

        float dx = targetX - x;
        float dy = targetY - y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 0.1f) {
            pathIndex++;
        } else {
            move(deltaTime, dx / dist, dy / dist, maze);
        }
    } else {
        float dx = lastKnownPlayerCell.x + 0.5f - x;
        float dy = lastKnownPlayerCell.y + 0.5f - y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 0.1f) {
            move(deltaTime, dx / dist, dy / dist, maze);
        }
    }
}

/**
 * 移动鬼到新位置（带碰撞检测）
 */
void Ghost::move(float deltaTime, float targetDirX, float targetDirY, const Maze& maze) {
    // 根据当前状态选择速度
    float speed = patrolSpeed;
    switch (currentState) {
        case State::Chasing:
            speed = chaseSpeed;
            break;
        case State::Alert:
            speed = alertSpeed;
            break;
        case State::Patrol:
        default:
            speed = patrolSpeed;
            break;
    }
    float moveStep = speed * deltaTime;

    // 更新方向
    dirX = targetDirX;
    dirY = targetDirY;

    // 计算新位置
    float newX = x + dirX * moveStep;
    float newY = y + dirY * moveStep;

    // 碰撞检测（分别检查X和Y，允许沿墙滑行）
    if (!checkCollision(newX, y, maze)) {
        x = newX;
    } else if (currentState == State::Patrol) {
        // 巡逻时碰到墙立即换方向
        chooseRandomDirection();
    }

    if (!checkCollision(x, newY, maze)) {
        y = newY;
    } else if (currentState == State::Patrol) {
        chooseRandomDirection();
    }
}

/**
 * 碰撞检测
 */
bool Ghost::checkCollision(float newX, float newY, const Maze& maze) const {
    const float collisionRadius = 0.2f;

    // 检查四个角点
    if (maze.isWall(int(newX - collisionRadius), int(newY - collisionRadius))) return true;
    if (maze.isWall(int(newX + collisionRadius), int(newY - collisionRadius))) return true;
    if (maze.isWall(int(newX - collisionRadius), int(newY + collisionRadius))) return true;
    if (maze.isWall(int(newX + collisionRadius), int(newY + collisionRadius))) return true;

    return false;
}

/**
 * 选择一个新的随机游荡方向
 */
void Ghost::chooseRandomDirection() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 3);

    // 四个基本方向：上、下、左、右
    int direction = dis(gen);
    switch (direction) {
        case 0: dirX =  0.0f; dirY = -1.0f; break;  // 上
        case 1: dirX =  0.0f; dirY =  1.0f; break;  // 下
        case 2: dirX = -1.0f; dirY =  0.0f; break;  // 左
        case 3: dirX =  1.0f; dirY =  0.0f; break;  // 右
    }
}

/**
 * A*寻路算法实现
 *
 * @param targetX 目标X坐标（格子）
 * @param targetY 目标Y坐标（格子）
 * @param maze 迷宫对象
 * @return 路径点序列（从起点到终点，不包括起点）
 */
std::vector<sf::Vector2i> Ghost::findPath(int targetX, int targetY, const Maze& maze) {
    // A*节点
    struct Node {
        int x, y;
        float g;  // 从起点到当前点的实际代价
        float h;  // 启发式估计（到终点的距离）
        float f;  // f = g + h
        int parentIndex;  // 父节点在allNodes中的索引（-1表示无）

        Node(int x_, int y_, float g_, float h_, int pIdx)
            : x(x_), y(y_), g(g_), h(h_), f(g_ + h_), parentIndex(pIdx) {}

        // 用于优先队列排序（f值小的优先）
        bool operator>(const Node& other) const {
            return f > other.f;
        }
    };

    // 起点和终点
    int startX = static_cast<int>(x);
    int startY = static_cast<int>(y);

    // 检查起点是否有效（鬼的位置必须有效）
    if (maze.isWall(startX, startY)) {
        return {};  // 鬼在墙里，无效
    }

    // 注意：不检查targetX, targetY是否是墙
    // 因为玩家可能站在墙边缘，网格坐标恰好在墙上
    // 我们仍然要尝试寻路到最近的可达位置

    // 已访问集合（用于快速查找）
    std::set<std::pair<int, int>> closedSet;

    // 优先队列（按f值排序）
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openQueue;

    // 启发式函数：曼哈顿距离
    auto heuristic = [targetX, targetY](int x, int y) -> float {
        return static_cast<float>(std::abs(x - targetX) + std::abs(y - targetY));
    };

    // 添加起点
    openQueue.emplace(startX, startY, 0.0f, heuristic(startX, startY), -1);

    // 存储所有节点（用于回溯路径）
    std::vector<Node> allNodes;
    allNodes.reserve(1000);  // 预留空间

    // A*主循环
    while (!openQueue.empty()) {
        // 取出f值最小的节点
        Node current = openQueue.top();
        openQueue.pop();

        // 检查是否已访问
        if (closedSet.count({current.x, current.y})) {
            continue;
        }

        // 标记为已访问
        closedSet.insert({current.x, current.y});

        // 保存节点（用于路径回溯）
        int currentIndex = static_cast<int>(allNodes.size());
        allNodes.push_back(current);

        // === 到达目标！ ===
        if (current.x == targetX && current.y == targetY) {
            // 回溯路径
            std::vector<sf::Vector2i> path;
            int idx = currentIndex;
            while (idx != -1 && allNodes[idx].parentIndex != -1) {
                path.push_back({allNodes[idx].x, allNodes[idx].y});
                idx = allNodes[idx].parentIndex;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        // === 扩展邻居节点（四个方向）===
        const int dx[] = {0, 0, -1, 1};
        const int dy[] = {-1, 1, 0, 0};

        for (int i = 0; i < 4; i++) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            // 检查是否在地图内且不是墙
            if (nx < 0 || ny < 0 || nx >= maze.getWidth() || ny >= maze.getHeight()) {
                continue;
            }
            if (maze.isWall(nx, ny)) {
                continue;
            }
            if (closedSet.count({nx, ny})) {
                continue;
            }

            // 计算新代价
            float newG = current.g + 1.0f;
            float newH = heuristic(nx, ny);

            // 添加到开放列表
            openQueue.emplace(nx, ny, newG, newH, currentIndex);
        }
    }

    // 未找到路径
    return {};
}

/**
 * 渲染鬼（俯视图）
 */
void Ghost::renderTopDown(sf::RenderWindow& window, float cellSize) const {
    // 绘制鬼的圆点
    float radius = cellSize * 0.25f;
    sf::CircleShape ghostCircle(radius);

    // 根据状态选择颜色
    switch (currentState) {
        case State::Chasing:
            ghostCircle.setFillColor(sf::Color::Red);  // 追逐：红色
            break;
        case State::Alert:
            ghostCircle.setFillColor(sf::Color(200, 120, 0));  // 警戒：橙色
            break;
        case State::Patrol:
        default:
            ghostCircle.setFillColor(sf::Color(100, 100, 100));  // 巡逻：灰色
            break;
    }

    ghostCircle.setOrigin({radius, radius});
    ghostCircle.setPosition({x * cellSize, y * cellSize});

    window.draw(ghostCircle);

    // 绘制路径（调试用）
    if ((currentState == State::Chasing || currentState == State::Alert) && !currentPath.empty()) {
        for (size_t i = 0; i < currentPath.size(); i++) {
            sf::RectangleShape pathDot({3, 3});
            pathDot.setFillColor(sf::Color::Yellow);
            pathDot.setPosition({
                currentPath[i].x * cellSize + cellSize / 2 - 1.5f,
                currentPath[i].y * cellSize + cellSize / 2 - 1.5f
            });
            window.draw(pathDot);
        }
    }
}

/**
 * 加载鬼的sprite纹理（所有鬼共享）
 */
bool Ghost::loadSpriteTexture(const std::string& filename) {
    if (s_textureLoaded) {
        return true;  // 已经加载过了
    }

    if (s_spriteTexture.loadFromFile(filename)) {
        s_textureLoaded = true;
        std::cout << "Ghost sprite loaded: " << filename << std::endl;
        return true;
    }

    std::cerr << "Failed to load ghost sprite: " << filename << std::endl;
    return false;
}

/**
 * 渲染鬼（第一人称视角）- Sprite渲染 + 深度测试
 *
 * 算法原理（DOOM式sprite渲染）：
 * 1. 计算鬼相对于玩家的位置（世界坐标 → 相机坐标）
 * 2. 投影到屏幕空间（计算屏幕X坐标和Y坐标）
 * 3. 根据距离计算sprite大小
 * 4. 逐列绘制sprite，并进行深度测试（只渲染比墙近的部分）
 */
void Ghost::renderFirstPerson(sf::RenderWindow& window, const Player& player,
                               int screenWidth, int screenHeight,
                               const std::vector<float>& zBuffer) const {
    if (!s_textureLoaded) {
        return;  // 没有加载纹理，不渲染
    }

    // === 步骤1：计算鬼相对于玩家的位置 ===
    float spriteX = x - player.getX();
    float spriteY = y - player.getY();

    // === 步骤2：变换到相机坐标系 ===
    float invDet = 1.0f / (player.getPlaneX() * player.getDirY() - player.getDirX() * player.getPlaneY());

    float transformX = invDet * (player.getDirY() * spriteX - player.getDirX() * spriteY);
    float transformY = invDet * (-player.getPlaneY() * spriteX + player.getPlaneX() * spriteY);

    // === 步骤3：检查sprite是否在玩家前方 ===
    if (transformY <= 0.1f) {
        return;  // 在玩家背后或太近，不渲染
    }

    // === 步骤4：计算sprite在屏幕上的位置和大小 ===
    int spriteScreenX = static_cast<int>((screenWidth / 2) * (1 + transformX / transformY));

    // 减小sprite大小，避免近距离扭曲（使用0.6倍缩放）
    int spriteHeight = static_cast<int>(screenHeight / transformY * 0.6f);
    int spriteWidth = spriteHeight;

    float horizon = screenHeight * 0.5f + player.getCameraOffsetY();
    if (horizon < 1.0f) {
        horizon = 1.0f;
    }
    if (horizon > screenHeight - 1.0f) {
        horizon = screenHeight - 1.0f;
    }
    int horizonY = static_cast<int>(horizon);

    // 调整垂直位置，让鬼"站"在地面上
    int drawStartY = -spriteHeight / 2 + horizonY + spriteHeight / 4;  // 向下偏移
    if (drawStartY < 0) drawStartY = 0;
    int drawEndY = spriteHeight / 2 + horizonY + spriteHeight / 4;
    if (drawEndY >= screenHeight) drawEndY = screenHeight - 1;

    int drawStartX = -spriteWidth / 2 + spriteScreenX;
    if (drawStartX < 0) drawStartX = 0;
    int drawEndX = spriteWidth / 2 + spriteScreenX;
    if (drawEndX >= screenWidth) drawEndX = screenWidth - 1;

    // === 步骤5：逐列绘制sprite并进行深度测试 ===
    sf::Vector2u texSize = s_spriteTexture.getSize();

    for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
        // === 深度测试：只渲染比墙近的sprite列 ===
        if (transformY >= zBuffer[stripe]) {
            continue;  // 这一列被墙挡住了，跳过
        }

        // 计算纹理X坐标
        int texX = static_cast<int>((stripe - (-spriteWidth / 2 + spriteScreenX)) * texSize.x / spriteWidth);

        // 绘制一列sprite
        sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);

        // 纹理坐标
        float texLeft = static_cast<float>(texX);
        float texRight = texLeft + 1.0f;

        // 根据距离调整亮度
        float brightness = 1.0f / (1.0f + transformY * 0.1f);
        brightness = std::max(0.3f, std::min(1.0f, brightness));

        // 打火机开启时降低鬼的亮度（让鬼更隐蔽）
        if (player.isLighterOn()) {
            brightness *= 0.5f;  // 降低到50%亮度
        }

        std::uint8_t colorValue = static_cast<std::uint8_t>(255 * brightness);
        sf::Color lightColor(colorValue, colorValue, colorValue);

        // 设置4个顶点（一列）
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

        // 绘制这一列
        sf::RenderStates states;
        states.texture = &s_spriteTexture;
        window.draw(quad, states);
    }
}

/**
 * 通知鬼听到了大声音（例如双胞胎发出的声音）
 *
 * @param soundLevel 声音强度
 * @param sourcePosition 声音源位置（格子坐标）
 */
void Ghost::notifyLoudSound(float soundLevel, sf::Vector2i sourcePosition) {
    // 如果声音足够大（超过听觉阈值），切换到追踪状态
    if (soundLevel > HEARING_THRESHOLD) {
        lastKnownPlayerCell = sourcePosition;

        if (currentState != State::Chasing) {
            transitionTo(State::Chasing);
            std::cout << "Ghost: Heard loud sound! Investigating..." << std::endl;
        }
    }
}

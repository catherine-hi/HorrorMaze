#pragma once
#include <SFML/Graphics.hpp>
#include <queue>
#include <vector>

class Maze;
class Player;

/**
 * Ghost类：AI敌人，具有声音感知和智能追踪能力
 *
 * 三种状态：
 * 1. Patrol（巡逻）：走走停停，随机移动
 * 2. Alert（警戒）：走走停停，朝最后位置靠近
 * 3. Chasing（追逐）：看见/听见玩家后奔跑追击
 */
class Ghost {
public:
    // 鬼的状态枚举
    enum class State {
        Patrol,   // 巡逻状态
        Alert,    // 警戒状态
        Chasing   // 追逐状态
    };

    // 构造函数
    Ghost(float startX, float startY);

    // 核心更新函数
    void update(float deltaTime, const Player& player, const Maze& maze);

    // 渲染函数
    void renderFirstPerson(sf::RenderWindow& window, const Player& player,
                          int screenWidth, int screenHeight,
                          const std::vector<float>& zBuffer) const;
    void renderTopDown(sf::RenderWindow& window, float cellSize) const;

    // 加载sprite纹理（静态方法，所有鬼共享）
    static bool loadSpriteTexture(const std::string& filename);
    static const sf::Texture* getSpriteTexture() { return &s_spriteTexture; }

    // 获取位置信息
    float getX() const { return x; }
    float getY() const { return y; }
    State getState() const { return currentState; }
    float getSpeed() const { return currentSpeed; }
    float getMaxSpeed() const { return chaseSpeed; }

    // 通知鬼听到了大声音（用于双胞胎等环境声音）
    void notifyLoudSound(float soundLevel, sf::Vector2i sourcePosition);

private:
    // === 位置和移动 ===
    float x, y;                    // 鬼的位置
    float dirX, dirY;              // 移动方向（单位向量）

    float patrolSpeed;             // 巡逻速度（比玩家慢）
    float alertSpeed;              // 警戒速度（中等）
    float chaseSpeed;              // 追逐速度（比玩家快）
    float currentSpeed;            // 当前速度（格/秒）

    // === AI状态 ===
    State currentState;            // 当前状态
    float stateChangeTimer;        // 状态切换冷却计时器
    static constexpr float STATE_CHANGE_COOLDOWN = 1.0f;  // 状态切换冷却时间1秒
    float alertTimer;              // 警戒持续计时器
    static constexpr float ALERT_DURATION = 4.0f;

    float movePauseTimer;          // 走走停停计时器
    bool movePaused;               // 是否暂停移动

    // === 声音感知系统 ===
    static constexpr float PLAYER_SOUND_WALK = 30.0f;        // 走路基础声音强度
    static constexpr float PLAYER_SOUND_RUN = 100.0f;        // 奔跑基础声音强度
    static constexpr float PLAYER_SOUND_CROUCH = 10.0f;      // 蹲走基础声音强度
    static constexpr float HEARING_THRESHOLD = 10.0f;        // 鬼的听觉阈值
    static constexpr float AIR_ATTENUATION = 0.08f;          // 空气中每单位距离的衰减系数（降低以增加范围）
    static constexpr float WALL_ATTENUATION_MULT = 0.3f;     // 穿墙衰减倍数（每堵墙 ×0.3）

    // === A*寻路 ===
    std::vector<sf::Vector2i> currentPath;  // 当前路径（格子坐标序列）
    int pathIndex;                          // 当前路径点索引
    float pathUpdateTimer;                  // 路径更新计时器
    static constexpr float PATH_UPDATE_INTERVAL = 0.5f;  // 每0.5秒更新一次路径

    sf::Vector2i lastKnownPlayerCell;       // 玩家最后一次被发现的位置
    float noPathWarningTimer;               // 无路径警告冷却计时器（避免刷屏）
    static constexpr float NO_PATH_WARNING_INTERVAL = 3.0f;  // 每3秒最多输出一次警告

    static constexpr float VISION_RANGE = 5.0f;
    static constexpr float PATROL_MOVE_DURATION = 1.6f;
    static constexpr float PATROL_PAUSE_DURATION = 0.8f;
    static constexpr float ALERT_MOVE_DURATION = 1.2f;
    static constexpr float ALERT_PAUSE_DURATION = 0.6f;

    // === 核心AI函数 ===

    /**
     * 计算鬼能听到的玩家声音强度
     *
     * 算法：
     * 1. 使用Bresenham直线算法追踪声音路径
     * 2. 计算距离衰减：S = S0 / (1 + distance * AIR_ATTENUATION)
     * 3. 每穿过一堵墙：S *= WALL_ATTENUATION_MULT
     *
     * @return 声音强度（0-100）
     */
    float calculateSoundLevel(const Player& player, const Maze& maze) const;
    bool canSeePlayer(const Player& player, const Maze& maze) const;

    /**
     * 检测玩家打火机光照（特殊视觉检测）
     *
     * 规则：
     * 1. 如果玩家打火机开启，光可以穿透1堵墙
     * 2. 如果中间没有墙，无论多远都能检测到
     *
     * @return true = 能检测到打火机光, false = 检测不到
     */
    bool canSeeLighter(const Player& player, const Maze& maze) const;

    /**
     * 使用A*算法计算从当前位置到目标的最短路径
     *
     * @param targetX 目标X坐标
     * @param targetY 目标Y坐标
     * @param maze 迷宫对象
     * @return 路径点序列（格子坐标）
     */
    std::vector<sf::Vector2i> findPath(int targetX, int targetY, const Maze& maze);

    /**
     * 追踪状态行为：沿着A*路径移动
     */
    void updateChasing(float deltaTime, const Player& player, const Maze& maze);

    /**
     * 巡逻状态行为：走走停停随机移动
     */
    void updatePatrol(float deltaTime, const Maze& maze);

    /**
     * 警戒状态行为：走走停停靠近最后位置
     */
    void updateAlert(float deltaTime, const Maze& maze);

    void transitionTo(State newState);

    /**
     * 移动鬼到新位置（带碰撞检测）
     */
    void move(float deltaTime, float targetDirX, float targetDirY, const Maze& maze);

    /**
     * 碰撞检测
     */
    bool checkCollision(float newX, float newY, const Maze& maze) const;

    /**
     * 选择一个新的随机游荡方向
     */
    void chooseRandomDirection();

    // === 静态纹理资源（所有鬼共享） ===
    static sf::Texture s_spriteTexture;
    static bool s_textureLoaded;
};

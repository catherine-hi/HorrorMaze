#pragma once
#include <SFML/Graphics.hpp>

// 前向声明（告诉编译器Maze类存在，但详细定义在Maze.h中）
class Maze;

/**
 * Player类：管理玩家的位置、移动、旋转
 *
 * 坐标系统：
 * - (x, y) 是玩家在迷宫中的位置（浮点数，可以在格子之间）
 * - 例如：(1.5, 2.3) 表示在第1-2格之间的x位置，第2-3格之间的y位置
 *
 * 方向向量：
 * - (dirX, dirY) 表示玩家面朝的方向
 * - 例如：(-1, 0) = 朝左, (1, 0) = 朝右, (0, -1) = 朝上, (0, 1) = 朝下
 */
class Player {
    enum class WallPhaseState;
public:
    enum class MoveMode {
        Walk,
        Run,
        Crouch
    };

    // 构造函数
    Player();
    Player(float startX, float startY);

    // 核心功能
    void update(float deltaTime, const Maze& maze);  // 每帧更新
    void move(float deltaTime, const Maze& maze);    // 处理移动
    void rotate(float angle);                        // 旋转视角

    // 获取玩家信息
    float getX() const { return x; }
    float getY() const { return y; }
    float getDirX() const { return dirX; }
    float getDirY() const { return dirY; }
    float getPlaneX() const { return planeX; }
    float getPlaneY() const { return planeY; }
    MoveMode getMoveMode() const { return m_moveMode; }
    float getCameraOffsetY() const { return m_cameraOffsetY; }
    float getStamina() const { return m_stamina; }
    float getStaminaMax() const { return m_staminaMax; }
    float getStaminaBaseMax() const { return STAMINA_BASE_MAX; }

    // 打火机功能
    bool hasLighter() const { return m_hasLighter; }
    bool isLighterOn() const { return m_lighterOn; }
    void toggleLighter();  // 开关打火机
    void disableLighter();  // 禁用打火机（强制关闭且无法使用）
    void enableLighter();   // 恢复打火机功能

    // 闪灵能力（Spirit Vision）
    bool isSpiritVisionActive() const { return m_spiritVisionActive; }
    float getSpiritVisionTimer() const { return m_spiritVisionTimer; }
    void activateSpiritVision();  // 激活闪灵
    void updateSpiritVision(float deltaTime);  // 更新闪灵状态

    // 钻雪墙能力（Wall Phase）
    bool isInWall() const { return m_inWall; }
    bool isEnteringWall() const { return m_wallPhaseState == WallPhaseState::Entering; }
    void toggleWallPhase(const Maze& maze);  // 切换钻墙/出墙状态

    // 冻结状态（双胞胎陷阱）
    void freeze() { m_frozen = true; }
    void unfreeze() { m_frozen = false; }
    bool isFrozen() const { return m_frozen; }

    // 渲染（俯视图）
    void renderTopDown(sf::RenderWindow& window, float cellSize) const;

private:
    // 位置和方向
    float x, y;              // 玩家位置
    float dirX, dirY;        // 方向向量（朝向）
    float planeX, planeY;    // 相机平面向量（用于3D渲染，现在先不用管）

    // 移动参数
    float moveSpeed;         // 移动速度（格/秒）
    float rotateSpeed;       // 旋转速度（弧度/秒）
    MoveMode m_moveMode;     // 当前移动模式
    float m_cameraOffsetY;   // 视角垂直偏移（像素）
    float m_cameraOffsetTargetY;

    // 输入状态（记录哪些键被按下）
    bool movingForward;      // W或↑键
    bool movingBackward;     // S或↓键
    bool strafingLeft;       // A或←键
    bool strafingRight;      // D或→键

    // 打火机状态
    bool m_hasLighter;       // 是否拥有打火机
    bool m_lighterOn;        // 打火机是否打开
    bool m_lighterDisabled;  // 打火机是否被禁用（双胞胎陷阱时禁用）

    // 闪灵状态
    bool m_spiritVisionActive;    // 闪灵是否激活
    float m_spiritVisionTimer;    // 闪灵持续时间计时器
    static constexpr float SPIRIT_VISION_DURATION = 3.0f;  // 持续3秒

    static constexpr float WALK_SPEED = 2.0f;
    static constexpr float RUN_SPEED = 4.0f;
    static constexpr float CROUCH_SPEED = 1.0f;
    static constexpr float CROUCH_CAMERA_OFFSET = 40.0f;
    static constexpr float CAMERA_OFFSET_LERP = 10.0f;

    static constexpr float STAMINA_BASE_MAX = 100.0f;
    static constexpr float STAMINA_RUN_DRAIN = 20.0f;
    static constexpr float STAMINA_RECOVER = 10.0f;
    static constexpr float STAMINA_WALL_MAX_DECAY = 5.0f;

    // 钻雪墙状态
    enum class WallPhaseState {
        None,
        Entering,
        Exiting
    };

    bool m_inWall;                // 是否在墙内
    float m_wallEntryX, m_wallEntryY;  // 钻墙前的位置
    float m_wallEntryDirX, m_wallEntryDirY;  // 钻墙前的朝向
    float m_wallEntryPlaneX, m_wallEntryPlaneY;  // 钻墙前的相机平面
    static constexpr float WALL_PHASE_DISTANCE = 0.8f;  // 钻墙检测距离
    static constexpr float WALL_PHASE_ANIM_DURATION = 0.4f;
    static constexpr float WALL_PHASE_STEP_DISTANCE = 0.3f;
    WallPhaseState m_wallPhaseState;
    float m_wallPhaseTimer;
    float m_wallPhaseRotateRemaining;

    // 体力系统
    float m_stamina;
    float m_staminaMax;

    // 冻结状态
    bool m_frozen;  // 是否被双胞胎冻结

    // 私有辅助函数
    void handleInput();      // 检查键盘输入
    bool checkCollision(float newX, float newY, const Maze& maze) const;  // 碰撞检测
    void updateWallPhase(float deltaTime);
    void updateStamina(float deltaTime, bool isMoving);
};


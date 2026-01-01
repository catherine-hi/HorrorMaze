#include "Player.h"
#include "Maze.h"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <iostream>
#include <array>

/**
 * 默认构造函数：创建一个在(1.5, 1.5)位置的玩家
 */
Player::Player()
    : Player(1.5f, 1.5f)  // 调用下面的构造函数
{
}

/**
 * 构造函数：创建一个在指定位置的玩家
 *
 * @param startX 起始X坐标
 * @param startY 起始Y坐标
 */
Player::Player(float startX, float startY)
    : x(startX)
    , y(startY)
    , dirX(0.0f)           // 初始朝向：向下（南）
    , dirY(1.0f)
    , planeX(-0.66f)       // 相机平面（用于3D渲染，垂直于朝向）
    , planeY(0.0f)
    , moveSpeed(WALK_SPEED)
    , rotateSpeed(3.0f)    // 每秒旋转3弧度（约172度）
    , m_moveMode(MoveMode::Walk)
    , m_cameraOffsetY(0.0f)
    , m_cameraOffsetTargetY(0.0f)
    , movingForward(false)
    , movingBackward(false)
    , strafingLeft(false)
    , strafingRight(false)
    , m_hasLighter(true)   // 游戏开始时拥有打火机
    , m_lighterOn(false)   // 初始状态：关闭
    , m_lighterDisabled(false)  // 初始状态：未禁用
    , m_spiritVisionActive(false)   // 初始状态：未激活
    , m_spiritVisionTimer(0.0f)
    , m_inWall(false)      // 初始状态：不在墙内
    , m_wallEntryX(0.0f)
    , m_wallEntryY(0.0f)
    , m_wallEntryDirX(0.0f)
    , m_wallEntryDirY(0.0f)
    , m_wallEntryPlaneX(0.0f)
    , m_wallEntryPlaneY(0.0f)
    , m_wallPhaseState(WallPhaseState::None)
    , m_wallPhaseTimer(0.0f)
    , m_wallPhaseRotateRemaining(0.0f)
    , m_stamina(STAMINA_BASE_MAX)
    , m_staminaMax(STAMINA_BASE_MAX)
    , m_frozen(false)      // 初始状态：未冻结
{
    // 构造函数不打印消息，避免临时对象造成控制台spam
}

/**
 * 检查键盘输入，更新输入状态
 */
void Player::handleInput() {
    // 检查前进键（W或上箭头）
    movingForward = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
                   sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);

    // 检查后退键（S或下箭头）
    movingBackward = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);

    // 检查左移键（A或左箭头）
    strafingLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);

    // 检查右移键（D或右箭头）
    strafingRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
                   sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);

    bool runPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
    bool crouchPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);

    if (crouchPressed) {
        m_moveMode = MoveMode::Crouch;
    } else if (runPressed) {
        m_moveMode = MoveMode::Run;
    } else {
        m_moveMode = MoveMode::Walk;
    }
}

/**
 * 碰撞检测：检查玩家能否移动到新位置
 *
 * 原理：
 * 1. 把玩家看作一个小圆（半径0.2格）
 * 2. 检查圆的四个角是否碰到墙
 *
 * @param newX 新的X坐标
 * @param newY 新的Y坐标
 * @param maze 迷宫对象（用来查询是否是墙）
 * @return true = 会碰撞（不能移动）, false = 没碰撞（可以移动）
 */
bool Player::checkCollision(float newX, float newY, const Maze& maze) const {
    const float collisionRadius = 0.2f;  // 玩家碰撞体积半径

    // 检查玩家"小圆"的四个角点是否在墙内
    // 左上角
    if (maze.isWall(int(newX - collisionRadius), int(newY - collisionRadius))) {
        return true;  // 碰到墙了！
    }

    // 右上角
    if (maze.isWall(int(newX + collisionRadius), int(newY - collisionRadius))) {
        return true;
    }

    // 左下角
    if (maze.isWall(int(newX - collisionRadius), int(newY + collisionRadius))) {
        return true;
    }

    // 右下角
    if (maze.isWall(int(newX + collisionRadius), int(newY + collisionRadius))) {
        return true;
    }

    return false;  // 没碰到墙，可以移动
}

/**
 * 处理玩家移动
 *
 * @param deltaTime 上一帧到现在的时间（秒）
 * @param maze 迷宫对象
 */
void Player::move(float deltaTime, const Maze& maze) {
    // distance = speed * deltaTime
    float moveStep = moveSpeed * deltaTime;

    // 前进（W键）
    if (movingForward) {
        // 计算新位置（沿着朝向方向移动）
        float newX = x + dirX * moveStep;
        float newY = y + dirY * moveStep;

        // 分别检查X和Y方向的碰撞（允许沿墙滑行）
        if (!checkCollision(newX, y, maze)) {
            x = newX;
        }
        if (!checkCollision(x, newY, maze)) {
            y = newY;
        }
    }

    // 后退（S键）
    if (movingBackward) {
        float newX = x - dirX * moveStep;
        float newY = y - dirY * moveStep;

        if (!checkCollision(newX, y, maze)) {
            x = newX;
        }
        if (!checkCollision(x, newY, maze)) {
            y = newY;
        }
    }

    float strafeX = -dirY;
    float strafeY = dirX;

    if (strafingLeft && !strafingRight) {
        float newX = x - strafeX * moveStep;
        float newY = y - strafeY * moveStep;

        if (!checkCollision(newX, y, maze)) {
            x = newX;
        }
        if (!checkCollision(x, newY, maze)) {
            y = newY;
        }
    } else if (strafingRight && !strafingLeft) {
        float newX = x + strafeX * moveStep;
        float newY = y + strafeY * moveStep;

        if (!checkCollision(newX, y, maze)) {
            x = newX;
        }
        if (!checkCollision(x, newY, maze)) {
            y = newY;
        }
    }
}

/**
 * 旋转玩家视角
 *
 * 数学原理（2D旋转）：
 * 新X = 旧X * cos(角度) - 旧Y * sin(角度)
 * 新Y = 旧X * sin(角度) + 旧Y * cos(角度)
 *
 * @param angle 旋转角度（弧度，正数=逆时针，负数=顺时针）
 */
void Player::rotate(float angle) {
    // 旋转方向向量
    float oldDirX = dirX;
    dirX = dirX * cos(angle) - dirY * sin(angle);
    dirY = oldDirX * sin(angle) + dirY * cos(angle);

    // 旋转相机平面向量（用于3D渲染）
    float oldPlaneX = planeX;
    planeX = planeX * cos(angle) - planeY * sin(angle);
    planeY = oldPlaneX * sin(angle) + planeY * cos(angle);
}

/**
 * 每帧更新玩家状态
 *
 * @param deltaTime 上一帧到现在的时间（秒）
 * @param maze 迷宫对象
 */
void Player::update(float deltaTime, const Maze& maze) {
    // 冻结时锁定
    if (m_frozen) {
        return;
    }

    // 步骤1：检查键盘输入
    handleInput();

    float targetOffset = 0.0f;
    if (m_moveMode == MoveMode::Crouch) {
        targetOffset = -CROUCH_CAMERA_OFFSET;
    }
    m_cameraOffsetTargetY = targetOffset;
    float lerp = CAMERA_OFFSET_LERP * deltaTime;
    if (lerp > 1.0f) {
        lerp = 1.0f;
    }
    m_cameraOffsetY += (m_cameraOffsetTargetY - m_cameraOffsetY) * lerp;

    if (m_moveMode == MoveMode::Run && m_stamina <= 0.0f) {
        m_moveMode = MoveMode::Walk;
    }

    bool isMoving = movingForward || movingBackward || strafingLeft || strafingRight;
    updateStamina(deltaTime, isMoving);
    if (m_moveMode == MoveMode::Run && m_stamina <= 0.0f) {
        m_moveMode = MoveMode::Walk;
    }

    switch (m_moveMode) {
        case MoveMode::Run:
            moveSpeed = RUN_SPEED;
            break;
        case MoveMode::Crouch:
            moveSpeed = CROUCH_SPEED;
            break;
        case MoveMode::Walk:
        default:
            moveSpeed = WALK_SPEED;
            break;
    }

    updateWallPhase(deltaTime);
    if (m_wallPhaseState != WallPhaseState::None) {
        return;
    }
    if (m_inWall) {
        return;
    }

    // 步骤2：处理移动
    move(deltaTime, maze);
}

void Player::updateStamina(float deltaTime, bool isMoving) {
    if (m_inWall) {
        m_staminaMax -= STAMINA_WALL_MAX_DECAY * deltaTime;
        if (m_staminaMax < 0.0f) {
            m_staminaMax = 0.0f;
        }
        m_stamina += STAMINA_RECOVER * deltaTime;
    } else if (m_moveMode == MoveMode::Run && isMoving) {
        m_stamina -= STAMINA_RUN_DRAIN * deltaTime;
    } else {
        m_stamina += STAMINA_RECOVER * deltaTime;
    }

    if (m_stamina < 0.0f) {
        m_stamina = 0.0f;
    }
    if (m_stamina > m_staminaMax) {
        m_stamina = m_staminaMax;
    }
}

/**
 * 切换打火机开关状态
 */
void Player::toggleLighter() {
    if (m_hasLighter && !m_lighterDisabled) {
        m_lighterOn = !m_lighterOn;
        std::cout << "Lighter: " << (m_lighterOn ? "ON" : "OFF") << std::endl;
    } else if (m_lighterDisabled) {
        std::cout << "Lighter is disabled!" << std::endl;
    }
}

/**
 * 禁用打火机（双胞胎陷阱触发时）
 */
void Player::disableLighter() {
    m_lighterOn = false;  // 强制关闭
    m_lighterDisabled = true;  // 禁用
    std::cout << ">>> Lighter forcibly turned OFF and DISABLED! <<<" << std::endl;
}

/**
 * 恢复打火机功能（双胞胎陷阱结束后）
 */
void Player::enableLighter() {
    m_lighterDisabled = false;  // 恢复
    std::cout << "Lighter functionality restored." << std::endl;
}

/**
 * 激活闪灵能力
 */
void Player::activateSpiritVision() {
    if (!m_spiritVisionActive) {
        m_spiritVisionActive = true;
        m_spiritVisionTimer = SPIRIT_VISION_DURATION;
        std::cout << ">>> SPIRIT VISION ACTIVATED <<<" << std::endl;
    }
}

/**
 * 更新闪灵状态（每帧调用）
 */
void Player::updateSpiritVision(float deltaTime) {
    if (m_spiritVisionActive) {
        m_spiritVisionTimer -= deltaTime;
        if (m_spiritVisionTimer <= 0.0f) {
            m_spiritVisionActive = false;
            m_spiritVisionTimer = 0.0f;
            std::cout << "Spirit Vision deactivated." << std::endl;
        }
    }
}

/**
 * 渲染玩家（俯视图）
 *
 * 绘制：
 * 1. 一个红色圆点（玩家位置）
 * 2. 一条黄色线（表示朝向）
 *
 * @param window SFML窗口对象
 * @param cellSize 每个格子的像素大小
 */
void Player::renderTopDown(sf::RenderWindow& window, float cellSize) const {
    // 绘制玩家圆点
    float radius = cellSize * 0.3f;  // 圆的半径
    sf::CircleShape playerCircle(radius);

    playerCircle.setFillColor(sf::Color::Red);
    playerCircle.setOrigin({radius, radius});  // 设置中心点
    playerCircle.setPosition({x * cellSize, y * cellSize});

    window.draw(playerCircle);

    // 绘制方向指示线
    std::array<sf::Vertex, 2> line;
    line[0].position = sf::Vector2f(x * cellSize, y * cellSize);
    line[0].color = sf::Color::Yellow;
    line[1].position = sf::Vector2f((x + dirX * 0.5f) * cellSize, (y + dirY * 0.5f) * cellSize);
    line[1].color = sf::Color::Yellow;

    window.draw(line.data(), 2, sf::PrimitiveType::Lines);
}

/**
 * 切换钻墙/出墙状态
 *
 * 钻墙条件：
 * 1. 当前不在墙内
 * 2. 前方 WALL_PHASE_DISTANCE 距离内有墙
 *
 * 钻入效果：
 * - 保存当前位置和朝向
 * - 稍微往前移动（到墙边缘）
 * - 反转视角180度，看向钻入前的位置
 *
 * 出墙效果：
 * - 回到钻入前的位置和朝向
 */
void Player::toggleWallPhase(const Maze& maze) {
    if (m_frozen || m_wallPhaseState != WallPhaseState::None) {
        return;
    }

    if (!m_inWall) {
        // === 尝试钻入墙 ===

        // 检测前方是否有墙
        float checkX = x + dirX * WALL_PHASE_DISTANCE;
        float checkY = y + dirY * WALL_PHASE_DISTANCE;

        if (maze.isWall(static_cast<int>(checkX), static_cast<int>(checkY))) {
            // 前方有墙，可以钻入
            std::cout << ">>> Phasing into wall... <<<" << std::endl;

            // 保存钻墙前的状态
            m_wallEntryX = x;
            m_wallEntryY = y;
            m_wallEntryDirX = dirX;
            m_wallEntryDirY = dirY;
            m_wallEntryPlaneX = planeX;
            m_wallEntryPlaneY = planeY;

            m_wallPhaseState = WallPhaseState::Entering;
            m_wallPhaseTimer = 0.0f;
            m_wallPhaseRotateRemaining = std::acos(-1.0f);
        } else {
            std::cout << "No wall ahead to phase into!" << std::endl;
        }
    } else {
        // === 退出墙 ===
        std::cout << ">>> Phasing out of wall... <<<" << std::endl;

        m_wallPhaseState = WallPhaseState::Exiting;
        m_wallPhaseTimer = 0.0f;
    }
}

void Player::updateWallPhase(float deltaTime) {
    if (m_wallPhaseState == WallPhaseState::Entering) {
        float rotateStep = (std::acos(-1.0f) / WALL_PHASE_ANIM_DURATION) * deltaTime;
        if (rotateStep > m_wallPhaseRotateRemaining) {
            rotateStep = m_wallPhaseRotateRemaining;
        }
        rotate(rotateStep);
        m_wallPhaseRotateRemaining -= rotateStep;

        if (m_wallPhaseRotateRemaining <= 0.0f) {
            x += m_wallEntryDirX * WALL_PHASE_STEP_DISTANCE;
            y += m_wallEntryDirY * WALL_PHASE_STEP_DISTANCE;
            m_inWall = true;
            m_wallPhaseState = WallPhaseState::None;
        }
        return;
    }

    if (m_wallPhaseState == WallPhaseState::Exiting) {
        m_wallPhaseTimer += deltaTime;
        if (m_wallPhaseTimer >= WALL_PHASE_ANIM_DURATION) {
            x = m_wallEntryX;
            y = m_wallEntryY;
            m_inWall = false;
            m_wallPhaseState = WallPhaseState::None;
        }
    }
}


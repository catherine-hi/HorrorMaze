#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <memory>
#include <vector>
#include <optional>
#include "Maze.h"      // 包含迷宫类
#include "Player.h"    // 包含玩家类
#include "Renderer.h"  // 包含渲染器类
#include "Ghost.h"     // 包含鬼类
#include "Twin.h"      // 包含双胞胎类

enum class GameState {
    Menu,
    Playing,
    Paused,
    GameOver,
    Victory
};

enum class ViewMode {
    TopDown,      // 俯视图
    FirstPerson   // 第一人称
};

enum class DeathCause {
    None,         // 未死亡
    Chopped,      // 被鬼抓到
    Frozen        // 时间耗尽，冻死
};

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    void processEvents();
    void update(float deltaTime);
    void render();

    void handleMenuInput(const sf::Event& event);
    void handleGameInput(const sf::Event& event);
    void switchView(); // 切换视角

    // 窗口
    sf::RenderWindow window;

    // 游戏状态
    GameState gameState;
    ViewMode viewMode;
    DeathCause deathCause;  // 死亡原因

    // 游戏数据
    int score;
    int lives;
    int currentLevel;
    float gameTimer;  // 游戏剩余时间（秒）
    static constexpr float GAME_TIME_LIMIT = 300.0f;  // 5分钟时间限制

    // 游戏对象
    Maze maze;      // 迷宫
    Player player;  // 玩家
    Renderer renderer;  // 渲染器
    std::vector<Ghost> ghosts;  // 鬼的列表
    std::vector<Twin> twins;    // 双胞胎陷阱列表

    // 双胞胎冻结状态
    bool playerFrozen;           // 玩家是否被冻结
    float frozenTimer;           // 冻结剩余时间
    static constexpr float FREEZE_DURATION = 4.0f;  // 冻结持续4秒
    int activeTwinIndex;         // 当前导致硬控的双胞胎索引（-1表示无）

    // 字体
    sf::Font font;  // 游戏字体
    bool fontLoaded;  // 字体是否加载成功

    // 声音系统
    sf::Music backgroundMusic;  // 背景音乐
    sf::Music heresJohnnyMusic;  // "Here's Johnny" 音效（使用Music以便游戏结束后继续播放）
    bool heresJohnnyLoaded;  // "Here's Johnny" 音效是否加载成功
    sf::SoundBuffer ghostStepBuffer;  // 鬼脚步声缓冲
    std::optional<sf::Sound> ghostStepSound;  // 鬼脚步声（延迟构造）
    sf::SoundBuffer twinVoice1Buffer;  // 双胞胎台词1缓冲
    sf::SoundBuffer twinVoice2Buffer;  // 双胞胎台词2缓冲
    std::optional<sf::Sound> twinVoiceSound;  // 双胞胎台词音效（延迟构造）
    int twinEncounterCount;  // 双胞胎遭遇次数（0=未遇到，1=第一次，2=第二次，3+=随机）
    bool soundsLoaded;  // 声音是否加载成功

    // 声音辅助函数
    void loadSounds();  // 加载所有声音
    void updateGhostFootsteps(float deltaTime);  // 更新鬼脚步声
    float footstepIntensity;
    float footstepAngle;

    // 闪灵相关
    std::vector<sf::Vector2i> escapePath;  // 逃生路径（A*计算）
    static constexpr float SPIRIT_VISION_TRIGGER_DISTANCE = 5.0f;  // 触发距离（格）

    // A*寻路算法
    std::vector<sf::Vector2i> findPathToExit(sf::Vector2i start, sf::Vector2i goal);

    // 常量
    static constexpr int WINDOW_WIDTH = 1200;
    static constexpr int WINDOW_HEIGHT = 800;
    static constexpr float TARGET_FPS = 60.0f;
    static constexpr float MOUSE_SENSITIVITY = 0.000833333f;
};

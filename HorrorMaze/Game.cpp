#include "Game.h"
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <set>
#include <cmath>
#include <cstdint>
#include <SFML/Window/Mouse.hpp>
#include <stdexcept>

Game::Game()
    : window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Horror Maze")
    , gameState(GameState::Menu)
    , viewMode(ViewMode::FirstPerson)  // 初始视角设为第一人称
    , deathCause(DeathCause::None)  // 初始未死亡
    , score(0)
    , lives(3)
    , currentLevel(1)
    , gameTimer(GAME_TIME_LIMIT)  // 初始化为5分钟
    , renderer(WINDOW_WIDTH, WINDOW_HEIGHT)
    , playerFrozen(false)  // 初始未冻结
    , frozenTimer(0.0f)
    , activeTwinIndex(-1)
    , fontLoaded(false)  // 初始化字体加载状态
    , heresJohnnyLoaded(false)  // 初始化 Here's Johnny 音效加载状态
    , soundsLoaded(false)  // 初始化声音加载状态
    , footstepIntensity(0.0f)
    , footstepAngle(0.0f)
    , twinEncounterCount(0)  // 初始化双胞胎遭遇次数
{
    window.setFramerateLimit(static_cast<unsigned int>(TARGET_FPS));

    // === 加载字体 ===
    std::vector<std::string> fontPaths = {
        "C:/Windows/Fonts/arial.ttf",  // Windows系统字体
        "C:/Windows/Fonts/simhei.ttf",  // 黑体（支持中文）
        "assets/fonts/arial.ttf",  // 项目字体
        "../../assets/fonts/arial.ttf"
    };

    for (const auto& path : fontPaths) {
        if (font.openFromFile(path)) {
            fontLoaded = true;
            std::cout << "Font loaded: " << path << std::endl;
            break;
        }
    }

    if (!fontLoaded) {
        std::cerr << "WARNING: Could not load any font!" << std::endl;
        std::cerr << "Text will not be displayed." << std::endl;
    }

    // === 加载声音 ===
    loadSounds();

    // 加载地图（尝试多个可能的路径）
    std::cout << "=== Horror Maze - Loading ===" << std::endl;
    std::cout << "Loading map..." << std::endl;

    bool loaded = false;
    std::vector<std::string> possiblePaths = {
        "assets/maps/level1.txt",                                                    // 当前目录
        "../../assets/maps/level1.txt",                                             // 从Debug目录回到根目录
        "E:/cs106A data structures/Final_Project/HorrorMazeFinal/assets/maps/level1.txt" // 绝对路径
    };

    for (const auto& path : possiblePaths) {
        std::cout << "Trying: " << path << std::endl;
        if (maze.loadFromFile(path)) {
            loaded = true;
            std::cout << "Map loaded successfully!" << std::endl;
            break;
        }
    }

    if (!loaded) {
        std::cerr << "ERROR: Cannot load map!" << std::endl;
        std::cerr << "Please ensure map file exists." << std::endl;
        throw std::runtime_error("Map file missing or unreadable.");
    }

    // 初始化玩家位置
    sf::Vector2i startPos = maze.getPlayerStart();
    player = Player(startPos.x + 0.5f, startPos.y + 0.5f);
    std::cout << "Player spawned at: (" << startPos.x << ", " << startPos.y << ")" << std::endl;

    // 初始化鬼：在迷宫左下或右上1/4区域的道路上随机刷新
    ghosts.clear();

    // 随机决定在左下还是右上区域生成
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> regionChoice(0, 1);
    bool spawnInBottomLeft = (regionChoice(gen) == 0);

    // 找到对应区域(1/4)的所有可行走位置
    std::vector<sf::Vector2i> validSpawnPositions;
    int halfWidth = maze.getWidth() / 2;
    int halfHeight = maze.getHeight() / 2;

    if (spawnInBottomLeft) {
        // 左下区域：x: [0, width/2), y: [height/2, height)
        std::cout << "Spawning ghost in BOTTOM-LEFT quarter..." << std::endl;
        for (int y = halfHeight; y < maze.getHeight(); y++) {
            for (int x = 0; x < halfWidth; x++) {
                if (!maze.isWall(x, y)) {
                    validSpawnPositions.push_back({x, y});
                }
            }
        }
    } else {
        // 右上区域：x: [width/2, width), y: [0, height/2)
        std::cout << "Spawning ghost in TOP-RIGHT quarter..." << std::endl;
        for (int y = 0; y < halfHeight; y++) {
            for (int x = halfWidth; x < maze.getWidth(); x++) {
                if (!maze.isWall(x, y)) {
                    validSpawnPositions.push_back({x, y});
                }
            }
        }
    }

    // 随机选择一个位置生成鬼
    if (!validSpawnPositions.empty()) {
        std::uniform_int_distribution<> dis(0, static_cast<int>(validSpawnPositions.size()) - 1);
        sf::Vector2i spawnPos = validSpawnPositions[dis(gen)];
        ghosts.emplace_back(spawnPos.x + 0.5f, spawnPos.y + 0.5f);
        std::cout << "Ghost spawned at: (" << spawnPos.x << ", " << spawnPos.y << ")" << std::endl;
    } else {
        std::cerr << "Warning: No valid spawn positions found for ghost!" << std::endl;
    }

    // 加载鬼的sprite纹理
    std::vector<std::string> spritePaths = {
        "assets/textures/ghost_sprite.png",
        "../../assets/textures/ghost_sprite.png",
        "E:/cs106A data structures/Final_Project/HorrorMazeFinal/assets/textures/ghost_sprite.png"
    };

    bool spriteLoaded = false;
    for (const auto& path : spritePaths) {
        if (Ghost::loadSpriteTexture(path)) {
            spriteLoaded = true;
            break;
        }
    }

    if (!spriteLoaded) {
        throw std::runtime_error("Required texture missing: ghost_sprite.png");
    }

    // 加载双胞胎的sprite纹理
    std::vector<std::string> twinSpritePaths = {
        "assets/textures/twin_sprite.png",
        "../../assets/textures/twin_sprite.png",
        "E:/cs106A data structures/Final_Project/HorrorMazeFinal/assets/textures/twin_sprite.png"
    };

    bool twinSpriteLoaded = false;
    for (const auto& path : twinSpritePaths) {
        if (Twin::loadSpriteTexture(path)) {
            twinSpriteLoaded = true;
            break;
        }
    }

    if (!twinSpriteLoaded) {
        throw std::runtime_error("Required texture missing: twin_sprite.png");
    }

    // === 初始化双胞胎陷阱 ===
    std::cout << "Spawning twins..." << std::endl;
    twins.clear();

    // 在迷宫中查找所有双胞胎标记位置（cell value = 3）
    int twinCount = 0;
    for (int y = 0; y < maze.getHeight(); y++) {
        for (int x = 0; x < maze.getWidth(); x++) {
            if (maze.getCell(x, y) == 3) {
                twins.emplace_back(x + 0.5f, y + 0.5f);
                twinCount++;
                std::cout << "  Twin #" << twinCount << " at grid (" << x << ", " << y << ")" << std::endl;
            }
        }
    }

    std::cout << "Total twins spawned: " << twins.size() << std::endl;

    std::cout << "Game initialized!" << std::endl;
    std::cout << "==============================" << std::endl;
}

Game::~Game() {
}

void Game::run() {
    sf::Clock clock;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        processEvents();
        update(deltaTime);
        render();
    }
};

void Game::processEvents() {
    while (auto event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }

        // ESC键退出
        if (const auto* keyPress = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPress->code == sf::Keyboard::Key::Escape) {
                window.close();
            }

            // Tab键切换视角
            if (keyPress->code == sf::Keyboard::Key::Tab && gameState == GameState::Playing) {
                switchView();
            }

            // F键切换打火机
            if (keyPress->code == sf::Keyboard::Key::F && gameState == GameState::Playing) {
                player.toggleLighter();
            }

            // E键钻墙/出墙
            if (keyPress->code == sf::Keyboard::Key::E && gameState == GameState::Playing) {
                player.toggleWallPhase(maze);
            }
        }

        // 根据游戏状态处理输入
        if (gameState == GameState::Menu || gameState == GameState::Victory || gameState == GameState::GameOver) {
            handleMenuInput(*event);  // 菜单、胜利和死亡界面都用同样的输入（Enter重新开始）
        }
        else if (gameState == GameState::Playing) {
            handleGameInput(*event);
        }
    }
}

void Game::handleMenuInput(const sf::Event& event) {
    if (const auto* keyPress = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPress->code == sf::Keyboard::Key::Enter ||
            keyPress->code == sf::Keyboard::Key::Space) {
            // 开始游戏或重新开始
            std::cout << "\n>>> Starting Game! <<<\n" << std::endl;
            gameState = GameState::Playing;
            deathCause = DeathCause::None;  // 重置死亡原因
            score = 0;
            lives = 3;
            currentLevel = 1;
            gameTimer = GAME_TIME_LIMIT;  // 重置计时器为5分钟

            // 播放背景音乐
            if (soundsLoaded && backgroundMusic.getStatus() != sf::SoundSource::Status::Playing) {
                backgroundMusic.play();
                std::cout << "Background music started." << std::endl;
            }

            // 重置玩家位置
            sf::Vector2i startPos = maze.getPlayerStart();
            player = Player(startPos.x + 0.5f, startPos.y + 0.5f);
            std::cout << "Player respawned at: (" << startPos.x << ", " << startPos.y << ")" << std::endl;

            // === 重新生成鬼（在左下或右上1/4区域随机刷新）===
            ghosts.clear();
            escapePath.clear();  // 清空逃生路径

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> regionChoice(0, 1);
            bool spawnInBottomLeft = (regionChoice(gen) == 0);

            // 找到对应区域的所有可行走位置
            std::vector<sf::Vector2i> validSpawnPositions;
            int halfWidth = maze.getWidth() / 2;
            int halfHeight = maze.getHeight() / 2;

            if (spawnInBottomLeft) {
                std::cout << "Spawning ghost in BOTTOM-LEFT quarter..." << std::endl;
                for (int y = halfHeight; y < maze.getHeight(); y++) {
                    for (int x = 0; x < halfWidth; x++) {
                        if (!maze.isWall(x, y)) {
                            validSpawnPositions.push_back({x, y});
                        }
                    }
                }
            } else {
                std::cout << "Spawning ghost in TOP-RIGHT quarter..." << std::endl;
                for (int y = 0; y < halfHeight; y++) {
                    for (int x = halfWidth; x < maze.getWidth(); x++) {
                        if (!maze.isWall(x, y)) {
                            validSpawnPositions.push_back({x, y});
                        }
                    }
                }
            }

            // 随机选择位置生成鬼
            if (!validSpawnPositions.empty()) {
                std::uniform_int_distribution<> dis(0, static_cast<int>(validSpawnPositions.size()) - 1);
                sf::Vector2i spawnPos = validSpawnPositions[dis(gen)];
                ghosts.emplace_back(spawnPos.x + 0.5f, spawnPos.y + 0.5f);
                std::cout << "Ghost spawned at: (" << spawnPos.x << ", " << spawnPos.y << ")" << std::endl;
            }

            // === 重置双胞胎陷阱 ===
            twins.clear();
            for (int y = 0; y < maze.getHeight(); y++) {
                for (int x = 0; x < maze.getWidth(); x++) {
                    if (maze.getCell(x, y) == 3) {
                        twins.emplace_back(x + 0.5f, y + 0.5f);
                    }
                }
            }
            std::cout << "Twins respawned: " << twins.size() << " traps" << std::endl;

            // 重置冻结状态
            playerFrozen = false;
            frozenTimer = 0.0f;

            window.setMouseCursorVisible(false);
            sf::Vector2u windowSize = window.getSize();
            sf::Mouse::setPosition(
                sf::Vector2i(static_cast<int>(windowSize.x / 2), static_cast<int>(windowSize.y / 2)),
                window
            );
        }
    }
}

void Game::handleGameInput(const sf::Event& event) {
    // 游戏中的输入处理（移动等）将在后面实现
}

void Game::update(float deltaTime) {
    if (gameState == GameState::Playing) {
        // === 更新双胞胎冻结状态 ===
        if (playerFrozen) {
            // 如果有激活的双胞胎，让玩家视角逐渐转向它
            if (activeTwinIndex >= 0 && activeTwinIndex < static_cast<int>(twins.size())) {
                const Twin& atwin = twins[activeTwinIndex];
                float dx = atwin.getX() - player.getX();
                float dy = atwin.getY() - player.getY();
                float targetAngle = std::atan2(dy, dx);
                float playerAngle = std::atan2(player.getDirY(), player.getDirX());

                // 计算角度差并归一到[-pi, pi]
                float diff = targetAngle - playerAngle;
                const float PI = std::acos(-1.0f);
                while (diff > PI) diff -= 2.0f * PI;
                while (diff < -PI) diff += 2.0f * PI;

                // 平滑转向，限制每帧最大旋转量
                float rotateSpeed = 2.5f; // rad/s
                float maxStep = rotateSpeed * deltaTime;
                if (std::abs(diff) > maxStep) {
                    diff = (diff > 0.0f) ? maxStep : -maxStep;
                }
                player.rotate(diff);
            }

            // 计时器递减并在结束时解除冻结
            frozenTimer -= deltaTime;
            if (frozenTimer <= 0.0f) {
                playerFrozen = false;
                frozenTimer = 0.0f;
                player.unfreeze();
                player.enableLighter();

                // 活动双胞胎索引重置
                activeTwinIndex = -1;

                sf::Vector2u windowSize = window.getSize();
                sf::Mouse::setPosition(
                    sf::Vector2i(static_cast<int>(windowSize.x / 2), static_cast<int>(windowSize.y / 2)),
                    window
                );

                std::cout << "Player unfrozen!" << std::endl;
            }
        }

        if (!playerFrozen && !player.isEnteringWall() && window.hasFocus()) {
            sf::Vector2u windowSize = window.getSize();
            sf::Vector2i center(static_cast<int>(windowSize.x / 2), static_cast<int>(windowSize.y / 2));
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            int deltaX = mousePos.x - center.x;
            if (deltaX != 0) {
                float angle = static_cast<float>(deltaX) * MOUSE_SENSITIVITY;
                player.rotate(angle);
            }
            sf::Mouse::setPosition(center, window);
        }

        // 更新玩家（处理移动）
        player.update(deltaTime, maze);

        // === 检测双胞胎触发 ===
        // 触发条件：玩家在视野内(±30°) OR 距离在2.5格内
        // 并且不能被墙阻挡（需要直视/直达）
        if (!playerFrozen) {
            for (size_t ti = 0; ti < twins.size(); ++ti) {
                auto& twin = twins[ti];
                if (twin.isActivated()) continue; // 已触发的跳过

                // 新触发条件：玩家和双胞胎在同一行或同一列，且中间没有墙
                int playerGX = static_cast<int>(player.getX());
                int playerGY = static_cast<int>(player.getY());
                int twinGX = static_cast<int>(twin.getX());
                int twinGY = static_cast<int>(twin.getY());

                bool sameLine = (playerGX == twinGX) || (playerGY == twinGY);
                if (!sameLine) {
                    continue; // 不在同一行或列，跳过
                }

                // 计算相对向量和距离（用于旋转/音量/日志）
                float dx = twin.getX() - player.getX();
                float dy = twin.getY() - player.getY();
                float dist = std::sqrt(dx * dx + dy * dy);

                // 检查是否被墙阻挡（Bresenham格线检测）
                int x0 = static_cast<int>(player.getX());
                int y0 = static_cast<int>(player.getY());
                int x1 = static_cast<int>(twin.getX());
                int y1 = static_cast<int>(twin.getY());

                bool blocked = false;
                int sx = (x0 < x1) ? 1 : -1;
                int sy = (y0 < y1) ? 1 : -1;
                int dxl = std::abs(x1 - x0);
                int dyl = std::abs(y1 - y0);
                int err = dxl - dyl;
                int cx = x0;
                int cy = y0;
                while (true) {
                    // 忽略起点和终点自身的格子（如果玩家或双胞胎正好在墙里会另外处理）
                    if (!(cx == x0 && cy == y0) && !(cx == x1 && cy == y1)) {
                        if (maze.isWall(cx, cy)) {
                            blocked = true;
                            break;
                        }
                    }

                    if (cx == x1 && cy == y1) break;

                    int e2 = 2 * err;
                    if (e2 > -dyl) { err -= dyl; cx += sx; }
                    if (e2 < dxl) { err += dxl; cy += sy; }
                }

                if (blocked) {
                    // 被墙阻挡，不能触发
                    continue;
                }

                // 触发硬控：根据遭遇次数选择台词
                twinEncounterCount++;  // 增加遭遇次数

                // 确定播放哪个台词
                sf::SoundBuffer* chosenBuffer = nullptr;
                if (twinEncounterCount == 1) {
                    // 第一次遭遇：播放 Twins1
                    chosenBuffer = &twinVoice1Buffer;
                    std::cout << "\n>>> FIRST TWIN ENCOUNTER! Playing Twins1.mp3 <<<" << std::endl;
                } else if (twinEncounterCount == 2) {
                    // 第二次遭遇：播放 Twins2
                    chosenBuffer = &twinVoice2Buffer;
                    std::cout << "\n>>> SECOND TWIN ENCOUNTER! Playing Twins2.mp3 <<<" << std::endl;
                } else {
                    // 第三次及以后：随机播放
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> dis(0, 1);
                    if (dis(gen) == 0) {
                        chosenBuffer = &twinVoice1Buffer;
                        std::cout << "\n>>> TWIN ENCOUNTER #" << twinEncounterCount << "! Randomly playing Twins1.mp3 <<<" << std::endl;
                    } else {
                        chosenBuffer = &twinVoice2Buffer;
                        std::cout << "\n>>> TWIN ENCOUNTER #" << twinEncounterCount << "! Randomly playing Twins2.mp3 <<<" << std::endl;
                    }
                }

                // 获取音频时长作为冻结时长
                float audioDuration = Twin::getDefaultSoundDuration(); // fallback
                if (chosenBuffer && chosenBuffer->getDuration().asSeconds() > 0.0f) {
                    audioDuration = chosenBuffer->getDuration().asSeconds();
                }

                twin.activate(audioDuration);
                playerFrozen = true;
                frozenTimer = audioDuration;
                player.freeze();
                player.disableLighter();
                activeTwinIndex = static_cast<int>(ti);

                // 播放选中的台词音效
                if (soundsLoaded && twinVoiceSound && chosenBuffer) {
                    // 如果之前正在播放，先停止
                    if (twinVoiceSound->getStatus() == sf::SoundSource::Status::Playing) {
                        twinVoiceSound->stop();
                    }
                    // 设置新的缓冲并播放
                    twinVoiceSound->setBuffer(*chosenBuffer);
                    twinVoiceSound->play();
                    std::cout << "Twin voice playing! duration=" << audioDuration << " seconds" << std::endl;
                }

                break; // 一次只触发一个
            }
        }

        // 更新所有双胞胎（声音衰减）
        for (auto& twin : twins) {
            twin.update(deltaTime);
        }

        // 更新玩家的闪灵状态
        player.updateSpiritVision(deltaTime);

        // 更新鬼脚步声（音量和立体声位置）
        updateGhostFootsteps(deltaTime);

        // 更新游戏计时器
        gameTimer -= deltaTime * 1.5f;
        if (gameTimer <= 0.0f) {
            // 时间耗尽，冻死
            std::cout << "\n*** YOU ARE FROZEN TO DEATH! ***\n" << std::endl;
            gameState = GameState::GameOver;
            deathCause = DeathCause::Frozen;
            window.setMouseCursorVisible(true);

            // 停止背景音乐
            if (backgroundMusic.getStatus() == sf::SoundSource::Status::Playing) {
                backgroundMusic.stop();
                std::cout << "Background music stopped." << std::endl;
            }
            return;
        }

        // 更新所有鬼（考虑双胞胎声音吸引）
        for (auto& ghost : ghosts) {
            // 检查是否有双胞胎发出的声音比玩家更响
            float loudestTwinSound = 0.0f;
            const Twin* loudestTwin = nullptr;

            for (const auto& twin : twins) {
                float twinSound = twin.getSoundLevel();
                if (twinSound > 0.0f) {
                    // 计算双胞胎到鬼的距离
                    float dx = twin.getX() - ghost.getX();
                    float dy = twin.getY() - ghost.getY();
                    float distance = std::sqrt(dx * dx + dy * dy);

                    // 空气衰减（和玩家声音一样）
                    float perceivedSound = twinSound / (1.0f + distance * 0.08f);

                    // === 计算声音路径上的墙数量（Bresenham直线算法）===
                    int x0 = static_cast<int>(ghost.getX());
                    int y0 = static_cast<int>(ghost.getY());
                    int x1 = static_cast<int>(twin.getX());
                    int y1 = static_cast<int>(twin.getY());

                    int wallCount = 0;
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

                    // === 应用墙壁衰减（和玩家声音一样，每堵墙×0.3）===
                    for (int i = 0; i < wallCount; i++) {
                        perceivedSound *= 0.3f;
                    }

                    if (perceivedSound > loudestTwinSound) {
                        loudestTwinSound = perceivedSound;
                        loudestTwin = &twin;
                    }
                }
            }

            // 如果双胞胎声音足够响（大于阈值15.0），让鬼追踪双胞胎位置
            if (loudestTwinSound > 15.0f && loudestTwin != nullptr) {
                // 通知鬼听到了双胞胎的声音（触发状态切换）
                ghost.notifyLoudSound(loudestTwinSound, {
                    static_cast<int>(loudestTwin->getX()),
                    static_cast<int>(loudestTwin->getY())
                });

                // 创建一个临时"玩家"位置代表双胞胎
                // 这样鬼会追向双胞胎而不是玩家
                Player twinTarget(loudestTwin->getX(), loudestTwin->getY());
                ghost.update(deltaTime, twinTarget, maze);
            } else {
                // 否则正常追踪玩家
                ghost.update(deltaTime, player, maze);
            }
        }

        // === 检测鬼距离，触发闪灵 ===
        if (!player.isSpiritVisionActive()) {  // 未激活时才检测
            for (const auto& ghost : ghosts) {
                float dx = ghost.getX() - player.getX();
                float dy = ghost.getY() - player.getY();
                float distance = std::sqrt(dx * dx + dy * dy);

                if (distance < SPIRIT_VISION_TRIGGER_DISTANCE) {
                    // 鬼靠近！激活闪灵
                    player.activateSpiritVision();

                    // 计算逃生路径
                    sf::Vector2i playerPos(static_cast<int>(player.getX()), static_cast<int>(player.getY()));
                    sf::Vector2i exitPos(18, 19);  // 根据地图获取终点位置

                    // 尝试从地图中找到终点（2=出口）
                    for (int y = 0; y < maze.getHeight(); y++) {
                        for (int x = 0; x < maze.getWidth(); x++) {
                            if (maze.getCell(x, y) == 2) {
                                exitPos = {x, y};
                                break;
                            }
                        }
                    }

                    escapePath = findPathToExit(playerPos, exitPos);
                    std::cout << "Escape path calculated: " << escapePath.size() << " steps" << std::endl;
                    break;  // 只需要触发一次
                }
            }
        }

        // === 检查鬼是否抓到玩家 ===
        // 玩家在墙内且关闭打火机时免疫碰撞检测
        // 如果在墙内但开着打火机，仍然会被抓到
        if (!player.isInWall() || player.isLighterOn()) {
            for (const auto& ghost : ghosts) {
                float dx = ghost.getX() - player.getX();
                float dy = ghost.getY() - player.getY();
                float distance = std::sqrt(dx * dx + dy * dy);

                // 增大碰撞范围到0.8格，更容易触发
                if (distance < 0.8f) {
                    std::cout << "\n*** YOU ARE CHOPPED! ***\n" << std::endl;
                    std::cout << "Ghost caught player at distance: " << distance << std::endl;

                    // 播放 "Here's Johnny" 音效（脚步声继续播放）
                    if (heresJohnnyLoaded) {
                        heresJohnnyMusic.play();
                        std::cout << "Here's Johnny!" << std::endl;
                    }

                    gameState = GameState::GameOver;
                    deathCause = DeathCause::Chopped;
                    window.setMouseCursorVisible(true);

                    // 停止背景音乐（但保留脚步声和 Here's Johnny 音效）
                    if (backgroundMusic.getStatus() == sf::SoundSource::Status::Playing) {
                        backgroundMusic.stop();
                        std::cout << "Background music stopped." << std::endl;
                    }
                    return;  // 立即停止更新
                }
            }
        } else {
            // 调试输出：玩家在墙内且打火机关闭，免疫碰撞
            static bool inWallWarningShown = false;
            if (!inWallWarningShown) {
                std::cout << "Player is in wall with lighter OFF - immune to ghost collision" << std::endl;
                inWallWarningShown = true;
            }
        }

        // === 检查玩家是否到达出口 ===
        int playerGridX = static_cast<int>(player.getX());
        int playerGridY = static_cast<int>(player.getY());

        if (maze.getCell(playerGridX, playerGridY) == 2) {
            // 到达出口！
            std::cout << "\n*** CONGRATULATIONS! You found the exit! ***\n" << std::endl;
            gameState = GameState::Victory;
            window.setMouseCursorVisible(true);
        }
    }
}

void Game::render() {
    window.clear(sf::Color::Black);

    if (gameState == GameState::Menu) {
        // 绘制菜单
        sf::RectangleShape menuBg({600, 400});
        menuBg.setFillColor(sf::Color(20, 20, 20));
        menuBg.setPosition({WINDOW_WIDTH / 2.0f - 300, WINDOW_HEIGHT / 2.0f - 200});
        window.draw(menuBg);

        if (fontLoaded) {
            // 标题
            sf::Text title(font);
            title.setString("HORROR MAZE");
            title.setCharacterSize(60);
            title.setFillColor(sf::Color::Red);
            title.setStyle(sf::Text::Bold);
            auto titleBounds = title.getLocalBounds();
            title.setPosition({
                WINDOW_WIDTH / 2.0f - titleBounds.size.x / 2,
                WINDOW_HEIGHT / 2.0f - 120
            });
            window.draw(title);

            // 提示文字
            sf::Text prompt(font);
            prompt.setString("Press ENTER to Start");
            prompt.setCharacterSize(30);
            prompt.setFillColor(sf::Color::Green);
            auto promptBounds = prompt.getLocalBounds();
            prompt.setPosition({
                WINDOW_WIDTH / 2.0f - promptBounds.size.x / 2,
                WINDOW_HEIGHT / 2.0f + 20
            });
            window.draw(prompt);

            // 控制说明
            sf::Text controls(font);
            controls.setString("WASD/Arrows: Move | Mouse: Look | Tab: Switch View | F: Lighter | E: Phase Wall");
            controls.setCharacterSize(18);
            controls.setFillColor(sf::Color(180, 180, 180));
            auto controlsBounds = controls.getLocalBounds();
            controls.setPosition({
                WINDOW_WIDTH / 2.0f - controlsBounds.size.x / 2,
                WINDOW_HEIGHT / 2.0f + 100
            });
            window.draw(controls);
        } else {
            // 备用：用矩形表示（如果字体加载失败）
            sf::RectangleShape titleBar({300, 60});
            titleBar.setFillColor(sf::Color(100, 0, 0));
            titleBar.setPosition({WINDOW_WIDTH / 2.0f - 150, WINDOW_HEIGHT / 2.0f - 100});
            window.draw(titleBar);

            sf::RectangleShape startButton({200, 40});
            startButton.setFillColor(sf::Color(0, 100, 0));
            startButton.setPosition({WINDOW_WIDTH / 2.0f - 100, WINDOW_HEIGHT / 2.0f});
            window.draw(startButton);
        }

    } else if (gameState == GameState::Playing) {
        // 根据视角模式选择渲染方式
        if (viewMode == ViewMode::TopDown) {
            // 俯视图渲染
            float cellSize = std::min(
                WINDOW_WIDTH / static_cast<float>(maze.getWidth()),
                WINDOW_HEIGHT / static_cast<float>(maze.getHeight())
            );
            renderer.renderTopDown(window, player, maze);

            // 渲染所有双胞胎
            for (const auto& twin : twins) {
                twin.renderTopDown(window, cellSize);
            }

            // 渲染所有鬼
            for (const auto& ghost : ghosts) {
                ghost.renderTopDown(window, cellSize);
            }

        } else {
            // 第一人称3D渲染
            renderer.renderFirstPerson(window, player, maze, escapePath);

            // 第一人称渲染双胞胎
            const auto& zBuffer = renderer.getZBuffer();
            for (const auto& twin : twins) {
                twin.renderFirstPerson(window, player, WINDOW_WIDTH, WINDOW_HEIGHT, zBuffer);
            }

            // 第一人称渲染鬼（sprite渲染 + 深度测试）
            for (const auto& ghost : ghosts) {
                ghost.renderFirstPerson(window, player, WINDOW_WIDTH, WINDOW_HEIGHT, zBuffer);
            }
        }

        // HUD（头顶显示信息）
        sf::RectangleShape hudBg({300, 120});
        hudBg.setFillColor(sf::Color(30, 30, 30, 200));
        hudBg.setPosition({WINDOW_WIDTH - 320.0f, 10});
        window.draw(hudBg);

        // 显示剩余时间（用色块表示分钟和秒）
        int minutes = static_cast<int>(gameTimer) / 60;
        int seconds = static_cast<int>(gameTimer) % 60;

        // 体力条（替换原状态条）
        float staminaBarWidth = 280.0f;
        float staminaBarHeight = 30.0f;
        float staminaX = WINDOW_WIDTH - 310.0f;
        float staminaY = 20.0f;

        sf::RectangleShape staminaBg({staminaBarWidth, staminaBarHeight});
        staminaBg.setFillColor(sf::Color(60, 60, 60));
        staminaBg.setPosition({staminaX, staminaY});
        window.draw(staminaBg);

        float staminaBase = player.getStaminaBaseMax();
        float staminaMax = player.getStaminaMax();
        float staminaCur = player.getStamina();
        if (staminaBase <= 0.0f) {
            staminaBase = 1.0f;
        }
        if (staminaMax < 0.0f) {
            staminaMax = 0.0f;
        }
        if (staminaMax > staminaBase) {
            staminaMax = staminaBase;
        }

        float maxWidth = staminaBarWidth * (staminaMax / staminaBase);
        float curWidth = staminaBarWidth * (staminaCur / staminaBase);

        if (maxWidth < staminaBarWidth) {
            sf::RectangleShape staminaLost({staminaBarWidth - maxWidth, staminaBarHeight});
            staminaLost.setFillColor(sf::Color(120, 200, 255));
            staminaLost.setPosition({staminaX + maxWidth, staminaY});
            window.draw(staminaLost);
        }

        if (curWidth > 0.0f) {
            sf::RectangleShape staminaFill({curWidth, staminaBarHeight});
            staminaFill.setFillColor(sf::Color(50, 200, 50));
            staminaFill.setPosition({staminaX, staminaY});
            window.draw(staminaFill);
        }

        // 数字显示（用小色块表示）
        // 分钟十位
        for (int i = 0; i < minutes / 10; i++) {
            sf::RectangleShape digit({8, 8});
            digit.setFillColor(sf::Color::White);
            digit.setPosition({WINDOW_WIDTH - 310.0f + i * 10, 60});
            window.draw(digit);
        }
        // 分钟个位
        for (int i = 0; i < minutes % 10; i++) {
            sf::RectangleShape digit({8, 8});
            digit.setFillColor(sf::Color::White);
            digit.setPosition({WINDOW_WIDTH - 260.0f + i * 10, 60});
            window.draw(digit);
        }
        // 冒号
        sf::RectangleShape colon1({4, 4});
        colon1.setFillColor(sf::Color::White);
        colon1.setPosition({WINDOW_WIDTH - 200.0f, 58});
        window.draw(colon1);
        sf::RectangleShape colon2({4, 4});
        colon2.setFillColor(sf::Color::White);
        colon2.setPosition({WINDOW_WIDTH - 200.0f, 66});
        window.draw(colon2);
        // 秒钟十位
        for (int i = 0; i < seconds / 10; i++) {
            sf::RectangleShape digit({8, 8});
            digit.setFillColor(sf::Color::White);
            digit.setPosition({WINDOW_WIDTH - 180.0f + i * 10, 60});
            window.draw(digit);
        }
        // 秒钟个位
        for (int i = 0; i < seconds % 10; i++) {
            sf::RectangleShape digit({8, 8});
            digit.setFillColor(sf::Color::White);
            digit.setPosition({WINDOW_WIDTH - 130.0f + i * 10, 60});
            window.draw(digit);
        }

        // 声纹指示器（鬼脚步方向）
        if (footstepIntensity > 0.0f) {
            sf::Vector2f ringCenter(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT * 0.35f);
            float ringRadius = 32.0f;
            sf::Vector2f offset(std::sin(footstepAngle) * ringRadius, -std::cos(footstepAngle) * ringRadius);

            sf::ConvexShape arrow(3);
            float arrowLength = 12.0f;
            float arrowWidth = 10.0f;
            arrow.setPoint(0, sf::Vector2f(0.0f, 0.0f));  // tip
            arrow.setPoint(1, sf::Vector2f(-arrowWidth * 0.5f, arrowLength));
            arrow.setPoint(2, sf::Vector2f(arrowWidth * 0.5f, arrowLength));
            arrow.setOrigin({0.0f, 0.0f});
            arrow.setPosition(ringCenter + offset);
            float angleDeg = footstepAngle * 180.0f / static_cast<float>(std::acos(-1.0f));
            arrow.setRotation(sf::degrees(angleDeg));

            std::uint8_t red = static_cast<std::uint8_t>(80 + 175 * footstepIntensity);
            arrow.setFillColor(sf::Color(red, 0, 0, 220));

            window.draw(arrow);
        }

    } else if (gameState == GameState::Victory) {
        // 胜利界面
        sf::RectangleShape victoryBg({800, 500});
        victoryBg.setFillColor(sf::Color(0, 80, 0));
        victoryBg.setPosition({WINDOW_WIDTH / 2.0f - 400, WINDOW_HEIGHT / 2.0f - 250});
        window.draw(victoryBg);

        if (fontLoaded) {
            // 胜利文字
            sf::Text victoryText(font);
            victoryText.setString("VICTORY!");
            victoryText.setCharacterSize(80);
            victoryText.setFillColor(sf::Color::Yellow);
            victoryText.setStyle(sf::Text::Bold);
            auto victoryBounds = victoryText.getLocalBounds();
            victoryText.setPosition({
                WINDOW_WIDTH / 2.0f - victoryBounds.size.x / 2,
                WINDOW_HEIGHT / 2.0f - 100
            });
            window.draw(victoryText);

            // 提示文字
            sf::Text restartText(font);
            restartText.setString("Press ENTER to Restart");
            restartText.setCharacterSize(30);
            restartText.setFillColor(sf::Color::White);
            auto restartBounds = restartText.getLocalBounds();
            restartText.setPosition({
                WINDOW_WIDTH / 2.0f - restartBounds.size.x / 2,
                WINDOW_HEIGHT / 2.0f + 50
            });
            window.draw(restartText);
        } else {
            // 备用矩形
            sf::RectangleShape restartHint({200, 40});
            restartHint.setFillColor(sf::Color::Yellow);
            restartHint.setPosition({WINDOW_WIDTH / 2.0f - 100, WINDOW_HEIGHT / 2.0f + 100});
            window.draw(restartHint);
        }

    } else if (gameState == GameState::GameOver) {
        // 游戏结束界面 - 根据死亡原因显示不同画面
        if (deathCause == DeathCause::Chopped) {
            // 被鬼抓到：YOU ARE CHOPPED
            sf::RectangleShape gameOverBg({900, 600});
            gameOverBg.setFillColor(sf::Color(80, 0, 0));  // 深红色背景
            gameOverBg.setPosition({WINDOW_WIDTH / 2.0f - 450, WINDOW_HEIGHT / 2.0f - 300});
            window.draw(gameOverBg);

            if (fontLoaded) {
                // 死亡文字
                sf::Text deathText(font);
                deathText.setString("YOU ARE CHOPPED!");
                deathText.setCharacterSize(70);
                deathText.setFillColor(sf::Color::Red);
                deathText.setStyle(sf::Text::Bold);
                auto deathBounds = deathText.getLocalBounds();
                deathText.setPosition({
                    WINDOW_WIDTH / 2.0f - deathBounds.size.x / 2,
                    WINDOW_HEIGHT / 2.0f - 100
                });
                window.draw(deathText);

                // 提示文字
                sf::Text restartText(font);
                restartText.setString("Press ENTER to Restart");
                restartText.setCharacterSize(30);
                restartText.setFillColor(sf::Color::Yellow);
                auto restartBounds = restartText.getLocalBounds();
                restartText.setPosition({
                    WINDOW_WIDTH / 2.0f - restartBounds.size.x / 2,
                    WINDOW_HEIGHT / 2.0f + 50
                });
                window.draw(restartText);
            } else {
                // 备用矩形
                sf::RectangleShape titleBar({600, 100});
                titleBar.setFillColor(sf::Color(200, 0, 0));
                titleBar.setPosition({WINDOW_WIDTH / 2.0f - 300, WINDOW_HEIGHT / 2.0f - 100});
                window.draw(titleBar);

                sf::RectangleShape restartHint({200, 40});
                restartHint.setFillColor(sf::Color::Yellow);
                restartHint.setPosition({WINDOW_WIDTH / 2.0f - 100, WINDOW_HEIGHT / 2.0f + 100});
                window.draw(restartHint);
            }

        } else if (deathCause == DeathCause::Frozen) {
            // 时间耗尽：YOU ARE FROZEN TO DEATH
            sf::RectangleShape gameOverBg({900, 600});
            gameOverBg.setFillColor(sf::Color(0, 40, 80));  // 深蓝色背景（冰冻感）
            gameOverBg.setPosition({WINDOW_WIDTH / 2.0f - 450, WINDOW_HEIGHT / 2.0f - 300});
            window.draw(gameOverBg);

            // 冰晶效果（用白色小方块模拟）
            for (int i = 0; i < 40; i++) {
                sf::RectangleShape ice({12, 12});
                ice.setFillColor(sf::Color(200, 220, 255, 100));
                ice.setPosition({
                    WINDOW_WIDTH / 2.0f - 400 + (i % 10) * 80,
                    WINDOW_HEIGHT / 2.0f - 250 + (i / 10) * 120
                });
                window.draw(ice);
            }

            if (fontLoaded) {
                // 死亡文字
                sf::Text deathText(font);
                deathText.setString("FROZEN TO DEATH");
                deathText.setCharacterSize(65);
                deathText.setFillColor(sf::Color::Cyan);
                deathText.setStyle(sf::Text::Bold);
                auto deathBounds = deathText.getLocalBounds();
                deathText.setPosition({
                    WINDOW_WIDTH / 2.0f - deathBounds.size.x / 2,
                    WINDOW_HEIGHT / 2.0f - 100
                });
                window.draw(deathText);

                // 提示文字
                sf::Text restartText(font);
                restartText.setString("Press ENTER to Restart");
                restartText.setCharacterSize(30);
                restartText.setFillColor(sf::Color::White);
                auto restartBounds = restartText.getLocalBounds();
                restartText.setPosition({
                    WINDOW_WIDTH / 2.0f - restartBounds.size.x / 2,
                    WINDOW_HEIGHT / 2.0f + 50
                });
                window.draw(restartText);
            } else {
                // 备用矩形
                sf::RectangleShape titleBar({700, 100});
                titleBar.setFillColor(sf::Color(100, 150, 200));
                titleBar.setPosition({WINDOW_WIDTH / 2.0f - 350, WINDOW_HEIGHT / 2.0f - 100});
                window.draw(titleBar);

                sf::RectangleShape restartHint({200, 40});
                restartHint.setFillColor(sf::Color::Yellow);
                restartHint.setPosition({WINDOW_WIDTH / 2.0f - 100, WINDOW_HEIGHT / 2.0f + 100});
                window.draw(restartHint);
            }
        }
    }

    window.display();
}

void Game::switchView() {
    if (viewMode == ViewMode::TopDown) {
        viewMode = ViewMode::FirstPerson;
        std::cout << "Switched to First-Person view" << std::endl;
    } else {
        viewMode = ViewMode::TopDown;
        std::cout << "Switched to Top-Down view" << std::endl;
    }
}

/**
 * 加载所有游戏声音
 */
void Game::loadSounds() {
    std::cout << "=== Loading Sounds ===" << std::endl;

    // === 加载背景音乐 ===
    std::vector<std::string> bgmPaths = {
        "assets/sounds/Background.mp3",
        "../../assets/sounds/Background.mp3",
        "E:/cs106A data structures/Final_Project/HorrorMazeFinal/assets/sounds/Background.mp3"
    };

    bool bgmLoaded = false;
    for (const auto& path : bgmPaths) {
        if (backgroundMusic.openFromFile(path)) {
            bgmLoaded = true;
            std::cout << "Background music loaded: " << path << std::endl;
            backgroundMusic.setLooping(true);  // 循环播放
            backgroundMusic.setVolume(30.0f);  // 设置音量为30%（不要太响）
            break;
        }
    }

    if (!bgmLoaded) {
        std::cerr << "WARNING: Could not load background music!" << std::endl;
    }

    // === 加载鬼脚步声 ===
    std::vector<std::string> stepPaths = {
        "assets/sounds/GhostStepSound.mp3",
        "../../assets/sounds/GhostStepSound.mp3",
        "E:/cs106A data structures/Final_Project/HorrorMazeFinal/assets/sounds/GhostStepSound.mp3"
    };

    bool stepLoaded = false;
    for (const auto& path : stepPaths) {
        if (ghostStepBuffer.loadFromFile(path)) {
            stepLoaded = true;
            std::cout << "Ghost step sound loaded: " << path << std::endl;

            // 构造sf::Sound对象
            ghostStepSound.emplace(ghostStepBuffer);
            ghostStepSound->setLooping(true);  // 循环播放脚步声
            ghostStepSound->setVolume(0.0f);  // 初始音量为0（会根据距离动态调整）
            ghostStepSound->setRelativeToListener(true);  // 相对于听者的位置
            ghostStepSound->play();  // 开始播放（但音量为0）
            break;
        }
    }

    if (!stepLoaded) {
        std::cerr << "WARNING: Could not load ghost step sound!" << std::endl;
    }

    // === 加载双胞胎台词音效 ===
    std::vector<std::string> twinVoice1Paths = {
        "assets/sounds/Twins1.mp3",
        "../../assets/sounds/Twins1.mp3",
        "E:/cs106A data structures/Final_Project/HorrorMazeFinal/assets/sounds/Twins1.mp3"
    };

    std::vector<std::string> twinVoice2Paths = {
        "assets/sounds/Twins2.mp3",
        "../../assets/sounds/Twins2.mp3",
        "E:/cs106A data structures/Final_Project/HorrorMazeFinal/assets/sounds/Twins2.mp3"
    };

    bool twinVoice1Loaded = false;
    for (const auto& path : twinVoice1Paths) {
        if (twinVoice1Buffer.loadFromFile(path)) {
            twinVoice1Loaded = true;
            std::cout << "Twin voice 1 loaded: " << path << std::endl;
            break;
        }
    }

    if (!twinVoice1Loaded) {
        std::cerr << "WARNING: Could not load Twins1.mp3!" << std::endl;
    }

    bool twinVoice2Loaded = false;
    for (const auto& path : twinVoice2Paths) {
        if (twinVoice2Buffer.loadFromFile(path)) {
            twinVoice2Loaded = true;
            std::cout << "Twin voice 2 loaded: " << path << std::endl;
            break;
        }
    }

    if (!twinVoice2Loaded) {
        std::cerr << "WARNING: Could not load Twins2.mp3!" << std::endl;
    }

    // 构造双胞胎台词音效对象（使用 Twins1 作为初始缓冲）
    if (twinVoice1Loaded) {
        twinVoiceSound.emplace(twinVoice1Buffer);
        twinVoiceSound->setVolume(80.0f);  // 台词音量较大
    }

    // === 加载 "Here's Johnny" 音效 ===
    std::vector<std::string> heresJohnnyPaths = {
        "assets/sounds/HeresJohnny.mp3",
        "../../assets/sounds/HeresJohnny.mp3",
        "E:/cs106A data structures/Final_Project/HorrorMazeFinal/assets/sounds/HeresJohnny.mp3"
    };

    heresJohnnyLoaded = false;  // 使用成员变量
    for (const auto& path : heresJohnnyPaths) {
        if (heresJohnnyMusic.openFromFile(path)) {
            heresJohnnyLoaded = true;
            std::cout << "Here's Johnny sound loaded: " << path << std::endl;
            heresJohnnyMusic.setVolume(100.0f);  // 满音量
            break;
        }
    }

    if (!heresJohnnyLoaded) {
        std::cerr << "WARNING: Could not load HeresJohnny.mp3!" << std::endl;
    }

    soundsLoaded = (bgmLoaded || stepLoaded || twinVoice1Loaded || twinVoice2Loaded || heresJohnnyLoaded);
    std::cout << "Sound loading complete." << std::endl;
    std::cout << "========================" << std::endl;
}

/**
 * 更新鬼脚步声的音量和立体声位置
 *
 * 算法：
 * 1. 找到距离玩家最近的鬼
 * 2. 根据距离和墙壁数量计算音量（使用和玩家声音相同的衰减公式）
 * 3. 根据鬼相对于玩家朝向的位置计算立体声（左右声道）
 */
void Game::updateGhostFootsteps(float deltaTime) {
    if (!soundsLoaded || ghosts.empty() || !ghostStepSound) {
        footstepIntensity = 0.0f;
        return;
    }

    // === 鬼脚步声参数（和玩家声音传播一样）===
    const float GHOST_FOOTSTEP_SOUND_WALK = 40.0f;   // 基础脚步声强度
    const float AIR_ATTENUATION = 0.08f;             // 空气衰减系数（和玩家一样）
    const float WALL_ATTENUATION_MULT = 0.3f;        // 墙壁衰减倍数（和玩家一样）
    const int MAX_WALL_COUNT = 2;                    // 最多穿透2堵墙

    float bestSoundLevel = 0.0f;
    const Ghost* loudestGhost = nullptr;
    float bestRelativeX = 0.0f;
    float bestRelativeY = 0.0f;

    for (const auto& ghost : ghosts) {
        // 只有移动中的鬼才发出脚步声
        float speed = ghost.getSpeed();
        if (speed <= 0.01f) {
            continue;
        }

        // 根据速度调整基础声音强度
        float maxSpeed = ghost.getMaxSpeed();
        float speedRatio = (maxSpeed > 0.0f) ? (speed / maxSpeed) : 0.0f;
        if (speedRatio > 1.0f) {
            speedRatio = 1.0f;
        }
        float baseSound = GHOST_FOOTSTEP_SOUND_WALK * speedRatio;

        // 计算距离
        float dx = ghost.getX() - player.getX();
        float dy = ghost.getY() - player.getY();
        float distance = std::sqrt(dx * dx + dy * dy);

        // === 空气衰减（和玩家声音一样）===
        float soundLevel = baseSound / (1.0f + distance * AIR_ATTENUATION);

        // === 计算声音路径上的墙数量（Bresenham直线算法）===
        int x0 = static_cast<int>(player.getX());
        int y0 = static_cast<int>(player.getY());
        int x1 = static_cast<int>(ghost.getX());
        int y1 = static_cast<int>(ghost.getY());

        int wallCount = 0;
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
                // 超过最大墙数，声音完全被阻挡
                if (wallCount > MAX_WALL_COUNT) {
                    soundLevel = 0.0f;
                    break;
                }
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

        // === 应用墙壁衰减（和玩家声音一样）===
        for (int i = 0; i < wallCount && i < MAX_WALL_COUNT; i++) {
            soundLevel *= WALL_ATTENUATION_MULT;
        }

        // 选择声音最响的鬼
        if (soundLevel > bestSoundLevel) {
            bestSoundLevel = soundLevel;
            loudestGhost = &ghost;
            bestRelativeX = dx;
            bestRelativeY = dy;
        }
    }

    // 如果没有听到脚步声，音量设为0
    if (!loudestGhost || bestSoundLevel <= 0.0f) {
        ghostStepSound->setVolume(0.0f);
        footstepIntensity = 0.0f;
        return;
    }

    // === 将声音强度转换为音量（0-100）===
    // 归一化：假设最大声音强度为50（近距离全速跑）
    float normalizedVolume = (bestSoundLevel / 50.0f) * 100.0f;
    normalizedVolume = std::max(0.0f, std::min(100.0f, normalizedVolume));

    ghostStepSound->setVolume(normalizedVolume);
    footstepIntensity = normalizedVolume / 100.0f;

    // === 计算立体声位置（3D音频）===
    float forward = bestRelativeX * player.getDirX() + bestRelativeY * player.getDirY();
    float right = bestRelativeX * (-player.getDirY()) + bestRelativeY * player.getDirX();
    ghostStepSound->setPosition({right * 2.0f, 0.0f, -forward * 2.0f});

    footstepAngle = std::atan2(right, forward);
}

/**
 * A*寻路算法：找到从起点到终点的最短路径
 */
std::vector<sf::Vector2i> Game::findPathToExit(sf::Vector2i start, sf::Vector2i goal) {
    struct Node {
        int x, y;
        float g, h, f;
        int parentIndex;  // 使用索引代替指针

        Node(int x_, int y_, float g_, float h_, int pIdx)
            : x(x_), y(y_), g(g_), h(h_), f(g_ + h_), parentIndex(pIdx) {}

        bool operator>(const Node& other) const {
            return f > other.f;
        }
    };

    // 检查起点和终点是否有效
    if (maze.isWall(start.x, start.y) || maze.isWall(goal.x, goal.y)) {
        return {};
    }

    std::set<std::pair<int, int>> closedSet;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openQueue;
    std::vector<Node> allNodes;
    allNodes.reserve(1000);  // 预留空间，减少重新分配

    auto heuristic = [goal](int x, int y) -> float {
        return static_cast<float>(std::abs(x - goal.x) + std::abs(y - goal.y));
    };

    // 添加起点（parentIndex = -1 表示没有父节点）
    openQueue.emplace(start.x, start.y, 0.0f, heuristic(start.x, start.y), -1);

    while (!openQueue.empty()) {
        Node current = openQueue.top();
        openQueue.pop();

        if (closedSet.count({current.x, current.y})) {
            continue;
        }

        closedSet.insert({current.x, current.y});

        // 保存当前节点并记录索引
        int currentIndex = static_cast<int>(allNodes.size());
        allNodes.push_back(current);

        // 到达目标
        if (current.x == goal.x && current.y == goal.y) {
            std::vector<sf::Vector2i> path;
            int idx = currentIndex;
            while (idx != -1) {
                path.push_back({allNodes[idx].x, allNodes[idx].y});
                idx = allNodes[idx].parentIndex;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        // 扩展邻居
        const int dx[] = {0, 0, -1, 1};
        const int dy[] = {-1, 1, 0, 0};

        for (int i = 0; i < 4; i++) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            if (nx < 0 || ny < 0 || nx >= maze.getWidth() || ny >= maze.getHeight()) {
                continue;
            }
            if (maze.isWall(nx, ny)) {
                continue;
            }
            if (closedSet.count({nx, ny})) {
                continue;
            }

            float newG = current.g + 1.0f;
            float newH = heuristic(nx, ny);

            openQueue.emplace(nx, ny, newG, newH, currentIndex);  // 使用索引
        }
    }

    return {};  // 未找到路径
}

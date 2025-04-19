#include "gamewindow.h"
#include <random>
#include <QPen>
#include <QBrush>
#include <QFont>

// 构造函数
GameWindow::GameWindow(QWidget *parent) : QWidget(parent)
{
    initUI();
    initGame();
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GameWindow::updateGame);
    timer->start(30);  // 启动定时器，每 30 毫秒触发一次超时信号
}

// 初始化用户界面
void GameWindow::initUI()
{
    setWindowTitle("第五喵格");
    setGeometry(100, 100, 800, 600);
    show();
}

// 初始化游戏元素
void GameWindow::initGame()
{
    //生成随机数
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(0, 700);
    std::uniform_int_distribution<> disY(0, 500);

    //加载player.png& killer.png
    playerPixmap.load(":/images/images/player.png");
    if (!playerPixmap.isNull())  playerPixmap = playerPixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    killerPixmap.load(":/images/images/killer.png");
    if(!killerPixmap.isNull()) killerPixmap=killerPixmap.scaled(20,20,Qt::KeepAspectRatio,Qt::SmoothTransformation);

    //初始化player,killer其他求生者（人机verson_1.0）
    player={disX(gen),disY(gen),5};
    isFlippingWindow = false;
    isFlippingBoard = false;
    flipWindowTime = 0;
    flipBoardTime = 0;
    killer = {disX(gen), disY(gen), 3};
    for (int i = 0; i < 3; i++)
    {
        survivors.push_back({disX(gen), disY(gen), 5});
        survivorEscaped.push_back(false);
    }

    // 初始化地图（板窗，密码机，地图，椅子）
    for (int i = 0; i < 3; ++i) windows.push_back({disX(gen), disY(gen), 0});
    for (int i = 0; i < 3; ++i)  boards.push_back({disX(gen), disY(gen), 0});
    for (int i = 0; i < 4; ++i) chairs.push_back({disX(gen), disY(gen), 0});
    gate.entity = {700, 500, 0};
    gate.progress = 0;
    gate.isOpen = false;
    for (int i = 0; i < 5; ++i)
    {
        CipherMachine cm;
        cm.entity = {disX(gen), disY(gen), 0};
        cm.progress = 0;
        cm.isBroken = false;
        cipherMachines.push_back(cm);
    }
}

// 更新游戏状态
void GameWindow::updateGame()
{
    //(更新Killer)killer追player的逻辑
    Entity newKiller=killer;
    killer.x<player.x?newKiller.x+=killer.speed:newKiller.x-=killer.speed;
    killer.y<player.y?newKiller.y+=killer.speed:newKiller.y-=killer.speed;
    if(!checkCollision(newKiller,player)) killer=newKiller;

    // 统计每个密码机正在破译的求生者数量
    std::vector<int> cipherMachineSurvivorCount(cipherMachines.size(), 0);
    bool gateBeingDecoded = false;

    //(更新survivors)求生者的逻辑
    for (size_t i = 0; i < survivors.size(); i++)
    {
        if (survivorEscaped[i]) continue;
        Entity newSurvivor = survivors[i];
        bool canMove = true;
        int unbrokenCount = 0;
        for (const auto& cm : cipherMachines) if (!cm.isBroken) unbrokenCount++;
        if (unbrokenCount > 0)
        {
            // 寻找最近的未破译密码机
            int closestIndex = -1;
            int minDistance = INT_MAX;
            for (size_t j = 0; j < cipherMachines.size(); ++j)
            {
                if (!cipherMachines[j].isBroken)
                {
                    int distance = std::abs(survivors[i].x - cipherMachines[j].entity.x) + std::abs(survivors[i].y - cipherMachines[j].entity.y);
                    if (distance < minDistance && cipherMachineSurvivorCount[j] < 4)
                    {
                        minDistance = distance;
                        closestIndex = j;
                    }
                }
            }

            if (closestIndex != -1)
            {
                // 朝最近的密码机移动
                if (survivors[i].x < cipherMachines[closestIndex].entity.x) newSurvivor.x += survivors[i].speed;
                else if (survivors[i].x > cipherMachines[closestIndex].entity.x) newSurvivor.x -= survivors[i].speed;
                if (survivors[i].y < cipherMachines[closestIndex].entity.y) newSurvivor.y += survivors[i].speed;
                else if (survivors[i].y > cipherMachines[closestIndex].entity.y) newSurvivor.y -= survivors[i].speed;

                // 检查是否可以破译密码机
                if (std::abs(survivors[i].x - cipherMachines[closestIndex].entity.x) < 50 && std::abs(survivors[i].y - cipherMachines[closestIndex].entity.y) < 50)
                {
                    // 可以破译，增加进度
                    cipherMachineSurvivorCount[closestIndex]++;
                    int speedBoost = cipherMachineSurvivorCount[closestIndex];
                    cipherMachines[closestIndex].progress += speedBoost;
                    if (cipherMachines[closestIndex].progress >= 100) cipherMachines[closestIndex].isBroken = true;
                    canMove = false;
                }
            }
        }
        else if (!gate.isOpen)
        {
            // 所有密码机已破译，前往破译大门
            if (survivors[i].x < gate.entity.x) newSurvivor.x += survivors[i].speed;
            else if (survivors[i].x > gate.entity.x) newSurvivor.x -= survivors[i].speed;
            if (survivors[i].y < gate.entity.y) newSurvivor.y += survivors[i].speed;
            else if (survivors[i].y > gate.entity.y) newSurvivor.y -= survivors[i].speed;

            // 检查是否可以破译大门
            if (std::abs(survivors[i].x - gate.entity.x) < 50 && std::abs(survivors[i].y - gate.entity.y) < 50 && !gateBeingDecoded)
            {
                // 可以破译，增加进度
                gate.progress += 2;
                if (gate.progress >= 100) gate.isOpen = true;
                gateBeingDecoded = true;
                canMove = false;
            }
        }
        else
        {
            // 大门已开启，前往逃脱
            if (survivors[i].x < gate.entity.x) newSurvivor.x += survivors[i].speed;
            else if (survivors[i].x > gate.entity.x) newSurvivor.x -= survivors[i].speed;
            if (survivors[i].y < gate.entity.y) newSurvivor.y += survivors[i].speed;
            else if (survivors[i].y > gate.entity.y) newSurvivor.y -= survivors[i].speed;

            // 检查是否逃脱
            if (checkCollision(newSurvivor, gate.entity))
            {
                survivorEscaped[i] = true;
                canMove = false;
            }
        }

        // 碰撞检测
        if (canMove)
        {
            if (checkCollision(newSurvivor, killer) || checkCollision(newSurvivor, player)) canMove = false;
            for (const auto& otherSurvivor : survivors) if (&survivors[i] != &otherSurvivor && checkCollision(newSurvivor, otherSurvivor)) canMove = false;
            if (canMove) survivors[i] = newSurvivor;
        }
    }

    //(更新player)处理翻窗，翻板
    if (isFlippingWindow)
    {
        flipWindowTime--;
        if (flipWindowTime <= 0) isFlippingWindow = false;
    }
    if (isFlippingBoard)
    {
        flipBoardTime--;
        if (flipBoardTime <= 0) isFlippingBoard = false;
    }

    update(); //调用paintEvent
}

// 碰撞检测function
bool GameWindow::checkCollision(const Entity& e1, const Entity& e2)
{
    return (e1.x < e2.x + 20 && e1.x + 20 > e2.x && e1.y < e2.y + 20 &&e1.y + 20 > e2.y);
}

//板窗交互 function
void GameWindow::moveToOtherSide(Entity& entity, const Entity& obstacle,int len=30)
{
    entity.x<obstacle.x? entity.x=obstacle.x+len:entity.x=obstacle.x-len;
    entity.y<obstacle.y? entity.y=obstacle.y+len:entity.y=obstacle.y-len;
}

// 绘制一帧
void GameWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);  // 创建绘图对象
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true); // 开启图片平滑渲染
    QPen pen(Qt::black);  // 创建画笔，颜色为黑色
    painter.setPen(pen);  // 设置画笔

    // 绘制player&survivors
    if (!survivorEscaped[0])  painter.drawPixmap(player.x, player.y, playerPixmap);
    for (size_t i = 0; i < survivors.size(); i++)
    {
        if (survivorEscaped[i]) continue;
        painter.drawRect(survivors[i].x, survivors[i].y, 20, 20);
        painter.setBrush(Qt::NoBrush);
    }

    // 绘制killer
    QBrush killerBrush(Qt::red);
    painter.setBrush(killerBrush);
    painter.drawPixmap(killer.x, killer.y, killerPixmap);
    painter.setBrush(Qt::NoBrush);

    // 绘制窗户
    QBrush windowBrush(Qt::yellow);
    painter.setBrush(windowBrush);
    for (const auto &window : windows)
    {
        painter.drawRect(window.x, window.y, 30, 30);
        //翻窗交互提示
        if (std::abs(player.x - window.x) < 50 && std::abs(player.y - window.y) < 50)
        {
            QFont font; //文本
            font.setPointSize(12);
            painter.setFont(font);
            painter.drawText(window.x, window.y - 20, "按空格翻窗");
        }
    }
    painter.setBrush(Qt::NoBrush);

    // 绘制板子
    QBrush boardBrush(Qt::green);
    painter.setBrush(boardBrush);
    for (const auto &board : boards)
    {
        painter.drawRect(board.x, board.y, 30, 10);
        if (std::abs(player.x - board.x) < 50 && std::abs(player.y - board.y) < 50)
        {
            QFont font;
            font.setPointSize(12);
            painter.setFont(font);
            painter.drawText(board.x, board.y - 20, "按空格翻板");
        }
    }
    painter.setBrush(Qt::NoBrush);

    // 绘制大门（darkYellow
    QBrush gateBrush(Qt::darkYellow);
    painter.setBrush(gateBrush);
    painter.drawRect(gate.entity.x, gate.entity.y, 50, 50);
    painter.setBrush(Qt::NoBrush);

    // 绘制密码机(gray)
    QBrush cipherMachineBrush(Qt::gray);
    painter.setBrush(cipherMachineBrush);
    for (const auto &cm : cipherMachines)
    {
        painter.drawRect(cm.entity.x, cm.entity.y, 20, 20);
        if (std::abs(player.x - cm.entity.x) < 50 && std::abs(player.y - cm.entity.y) < 50)
        {
            QFont font;
            font.setPointSize(12);
            painter.setFont(font);
            painter.drawText(cm.entity.x, cm.entity.y - 20, "按空格破译");
        }
        //进度条
        painter.setBrush(Qt::blue);
        painter.drawRect(cm.entity.x, cm.entity.y - 10, cm.progress / 5, 5);
        painter.setBrush(Qt::NoBrush);
    }

    // 绘制椅子
    for (const auto& chair : chairs)  painter.drawRect(chair.x, chair.y, 20, 20);

    // 显示未破译密码机数量
    int unbrokenCount = 0;
    for (const auto& cm : cipherMachines) if (!cm.isBroken) unbrokenCount++;
    QFont statusFont;
    statusFont.setPointSize(12);
    painter.setFont(statusFont);
    painter.drawText(10, 20, "未破译密码机数量: " + QString::number(unbrokenCount));

    // 显示所有求生者的状态
    QString survivorStatus = "求生者状态: ";
    for (size_t i = 0; i < survivors.size(); i++)
    {
        if (survivorEscaped[i]) survivorStatus += "逃脱 ";
        else survivorStatus += "参与 ";
    }
    painter.drawText(10, 40, survivorStatus);

    // 当所有密码机破译完成后，显示大门的破译提示和进度条
    if (unbrokenCount == 0)
    {
        //开门提示
        if (std::abs(player.x - gate.entity.x) < 50 && std::abs(player.y - gate.entity.y) < 50)
        {
            QFont font;
            font.setPointSize(12);
            painter.setFont(font);
            painter.drawText(gate.entity.x, gate.entity.y - 20, "按空格开门");
        }
        //进度条
        painter.setBrush(Qt::blue);
        painter.drawRect(gate.entity.x, gate.entity.y - 10, gate.progress / 5, 5);
        painter.setBrush(Qt::NoBrush);
    }

    // 显示胜利信息
    bool allSurvivorsEscaped = true;
    for (bool escaped : survivorEscaped)
    {
        if (!escaped)
        {
            allSurvivorsEscaped = false;
            break;
        }
    }
    if (allSurvivorsEscaped)
    {
        QFont winFont;
        winFont.setPointSize(30);
        painter.setFont(winFont);
        painter.drawText(300, 300, "大获全胜");
    }
}

// 处理键盘按键事件
void GameWindow::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    Entity newPlayer = player;
    // player移动
    if (key == Qt::Key_W) newPlayer.y -= player.speed;
    else if (key == Qt::Key_S) newPlayer.y += player.speed;
    else if (key == Qt::Key_A) newPlayer.x -= player.speed;
    else if (key == Qt::Key_D) newPlayer.x += player.speed;

    // 碰撞检测
    bool canMove = true;
    if (!isFlippingWindow &&!isFlippingBoard)
    {
        for (const auto &window : windows) if (checkCollision(newPlayer, window)) canMove = false;
        for (const auto &board : boards) if (checkCollision(newPlayer, board)) canMove = false;
        if (!gate.isOpen && checkCollision(newPlayer, gate.entity)) canMove = false;
        if (checkCollision(newPlayer, killer)) canMove = false;
        for (const auto &survivor : survivors) if (checkCollision(newPlayer, survivor)) canMove = false;
    }
    if (canMove) player = newPlayer;

    // 处理交互
    if (key == Qt::Key_Space)
    {
        //window
        for (const auto &window : windows)
        {
            if (std::abs(player.x - window.x) < 50 &&std::abs(player.y - window.y) < 50)
            {
                isFlippingWindow = true;
                flipWindowTime = 30;
                moveToOtherSide(player, window);
                break;
            }
        }
        //board
        for (const auto &board : boards)
        {
            if (std::abs(player.x - board.x) < 50 &&std::abs(player.y - board.y) < 50)
            {
                isFlippingBoard = true;
                flipBoardTime = 30;
                moveToOtherSide(player, board);
                break;
            }
        }
        // cipherMachine
        for (auto &cm : cipherMachines)
        {
            if (std::abs(player.x - cm.entity.x) < 50 && std::abs(player.y - cm.entity.y) < 50 &&!cm.isBroken)
            {
                cm.progress += 2;
                if (cm.progress >= 100) cm.isBroken = true;
            }
        }
        // gate
        int unbrokenCount = 0;
        for (const auto& cm : cipherMachines) if (!cm.isBroken) unbrokenCount++;
        if (unbrokenCount == 0 && std::abs(player.x - gate.entity.x) < 50 && std::abs(player.y - gate.entity.y) < 50 && !gate.isOpen)
        {
            gate.progress += 5;
            if (gate.progress >= 100) gate.isOpen = true;
        }

        // 处理逃脱
        if (gate.isOpen && checkCollision(player, gate.entity)) survivorEscaped[0] = true;
    }
    update();
}

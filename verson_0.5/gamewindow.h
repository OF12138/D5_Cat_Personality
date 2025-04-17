#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QKeyEvent>
#include <QPainter>
#include <QTimer>
#include <vector>

struct Entity
{
    int x;
    int y;
    int speed;
};

struct CipherMachine
{
    Entity entity;
    int progress;
    bool isBroken;
};

struct Gate
{
    Entity entity;
    int progress;
    bool isOpen;
};

class GameWindow : public QWidget
{
    Q_OBJECT
public:
    GameWindow(QWidget *parent = nullptr);
private:
    Entity player;
    std::vector<Entity> survivors;
    Entity killer;
    std::vector<Entity> windows;
    std::vector<Entity> boards;
    Gate gate;
    std::vector<CipherMachine> cipherMachines;
    std::vector<Entity> chairs;
    std::vector<bool> survivorOnChair;
    std::vector<bool> survivorEscaped;
    bool isFlippingWindow;
    bool isFlippingBoard;
    int flipWindowTime;
    int flipBoardTime;
    QTimer *timer;  // 定时器
    void initUI();
    void initGame();
    void updateGame();
    bool checkCollision(const Entity& e1, const Entity& e2);
    void moveToOtherSide(Entity& entity, const Entity& obstacle,int len);
protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
};

#endif

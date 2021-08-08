#ifndef PICARIA_H
#define PICARIA_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
    class Picaria;
}
QT_END_NAMESPACE

class Hole;

class Picaria : public QMainWindow {
    Q_OBJECT
    Q_PROPERTY(Picaria::Mode mode READ mode WRITE setMode NOTIFY modeChanged)

public:
    enum Mode {
        NineHoles,
        ThirteenHoles
    };
    Q_ENUM(Mode)

    enum Player {
        RedPlayer,
        BluePlayer
    };
    Q_ENUM(Player)

    enum Phase {
        DropPhase,
        MovePhase
    };
    Q_ENUM(Phase)

    enum Direction {
        North,
        Northeast,
        East,
        Southeast,
        South,
        Southwest,
        West,
        Northwest,
    };
    Q_ENUM(Direction)

    Picaria(QWidget *parent = nullptr);
    virtual ~Picaria();

    Picaria::Mode mode() const { return m_mode; }
    void setMode(Picaria::Mode mode);

signals:
    void modeChanged(Picaria::Mode mode);

signals:
    void gameOver(Player player);
private:
    Ui::Picaria *ui;
    Hole* m_holes[13];
    Mode m_mode;
    Player m_player;
    Phase m_phase;
    int m_dropCount;
    Hole* m_selected;

    void switchPlayer();
    QList<Hole*> findSelectable(Hole * hole);
    bool confereVitoria(Hole* hole1, Hole* hole2, Hole* hole3);
    bool isGameOver(Hole* hole);
    int getId(Hole* hole);

private slots:
    Hole* getNeighborHole(Hole* hole, Direction direction);
    int getNeighborId(int id, Direction direction);
    void play(int id);
    void move(Hole* hole);
    void drop(Hole* hole);
    void reset();

    void clearSelectable();
    void showAbout();

    void updateMode(QAction* action);
    void showGameOver(Player player);
    void updateStatusBar();

};

#endif // PICARIA_H

#include "Picaria.h"
#include "ui_Picaria.h"

#include <QDebug>
#include <QMessageBox>
#include <QActionGroup>
#include <QSignalMapper>

Picaria::Player state2player(Hole::State state) {
    switch (state) {
        case Hole::RedState:
            return Picaria::RedPlayer;
        case Hole::BlueState:
            return Picaria::BluePlayer;
        default:
            Q_UNREACHABLE();
    }
}

Hole::State player2state(Picaria::Player player) {
    return player == Picaria::RedPlayer ? Hole::RedState : Hole::BlueState;
}

Picaria::Picaria(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::Picaria),
      m_mode(Picaria::NineHoles),
      m_player(Picaria::RedPlayer),
      m_phase(Picaria::DropPhase),
      m_dropCount(0),
      m_selected(nullptr) {

    ui->setupUi(this);

    QActionGroup* modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);
    modeGroup->addAction(ui->action9holes);
    modeGroup->addAction(ui->action13holes);

    QObject::connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(reset()));
    QObject::connect(ui->actionQuit, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
    QObject::connect(modeGroup, SIGNAL(triggered(QAction*)), this, SLOT(updateMode(QAction*)));
    QObject::connect(this, SIGNAL(modeChanged(Picaria::Mode)), this, SLOT(reset()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAbout()));

    QSignalMapper* map = new QSignalMapper(this);
    for (int id = 0; id < 13; ++id) {
        QString holeName = QString("hole%1").arg(id+1, 2, 10, QChar('0'));
        Hole* hole = this->findChild<Hole*>(holeName);
        Q_ASSERT(hole != nullptr);

        m_holes[id] = hole;
        map->setMapping(hole, id);
        QObject::connect(hole, SIGNAL(clicked(bool)), map, SLOT(map()));
    }
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QObject::connect(map, SIGNAL(mapped(int)), this, SLOT(play(int)));
#else
    QObject::connect(map, SIGNAL(mappedInt(int)), this, SLOT(play(int)));
#endif

    this->reset();

    this->adjustSize();
    this->setFixedSize(this->size());
}

Picaria::~Picaria() {
    delete ui;
}

void Picaria::setMode(Picaria::Mode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        emit modeChanged(mode);
    }
}

void Picaria::switchPlayer() {
    m_player = m_player == Picaria::RedPlayer ?
                    Picaria::BluePlayer : Picaria::RedPlayer;
    this->updateStatusBar();
}

Hole* Picaria::getNeighborHole(int id, Picaria::Direction direction){
    int index = -1;

    int matrix[13][8] = {{-1,-1, 1, 2, 3,-1,-1,-1},
                  {-1,-1, 2, 4, 6, 3, 0,-1},
                  {-1,-1,-1,-1, 7, 4, 1,-1},
                  {-1, 1, 4, 6, 8, 5,-1, 0},
                  {-1, 2,-1, 7, 9, 6, 3, 1},
                  { 0, 3, 6, 8,10,-1,-1,-1},
                  { 1, 4, 7, 9,11, 8, 5, 3},
                  { 2,-1,-1,-1,12, 9, 6, 4},
                  { 3, 6, 9,11,-1,10,-1, 5},
                  { 4, 7,-1,12,-1,11, 8, 6},
                  { 5, 8,11,-1,-1,-1,-1,-1},
                  { 6, 9,12,-1,-1,-1,10, 8},
                  { 7,-1,-1,-1,-1,-1,11, 9}};

    index = matrix[id][direction];
    m_holes[id]->setState(Hole::SelectableState);
    qDebug() << index;
    if (index != -1) {
        return m_holes[index];
    } else{
        return 0;
    };
}

void Picaria::play(int id) {
    Hole* hole = m_holes[id];

    qDebug() << "clicked on: " << hole->objectName();

    switch (m_phase) {
            case Picaria::DropPhase:
                drop(hole);
                break;
            case Picaria::MovePhase:
                move(id);
                break;
            default:
                Q_UNREACHABLE();
        }
}

void Picaria::move(int id){
    QPair<Hole*,Hole*>* movement = nullptr;
    Hole* hole = m_holes[id];
    getNeighborHole(id,Picaria::North);
    if (hole->state() == Hole::SelectableState) {
        Q_ASSERT(m_selected != 0);
        movement = new QPair<Hole*,Hole*>(m_selected, hole);
    }
}

void Picaria::drop(Hole* hole) {
    if (hole->state() == Hole::EmptyState){
        hole->setState(player2state(m_player));

        if (true){
            ++m_dropCount;
            if (m_dropCount == 6)
                m_phase = Picaria::MovePhase;
            this->switchPlayer();
        }
    }
}

void Picaria::reset() {
    // Reset each hole.
    for (int id = 0; id < 13; ++id) {
        Hole* hole = m_holes[id];
        hole->reset();

        // Set the hole visibility according to the board mode.
        switch (id) {
            case 3:
            case 4:
            case 8:
            case 9:
                hole->setVisible(m_mode == Picaria::ThirteenHoles);
                break;
            default:
                break;
        }
    }

    // Reset the player and phase.
    m_player = Picaria::RedPlayer;
    m_phase = Picaria::DropPhase;

    // Finally, update the status bar.
    this->updateStatusBar();
}

void Picaria::clearSelectable() {
    for (int id = 0; id < 13; id++) {
        Hole* hole = m_holes[id];
        if (hole->state() == Hole::SelectableState)
            hole->setState(Hole::EmptyState);
    }
}

/*QList<Hole*> Picaria::findSelectable(Hole* hole) {
    QList<Hole*> list;

    Hole* left;
    int aux
    return list;
}*/

void Picaria::showAbout() {
    QMessageBox::information(this, tr("About"), tr("Picaria\n\nRodrigo Lopes - 20193017107\n\nHenrique Coelho - ___"));
}

void Picaria::updateMode(QAction* action) {
    if (action == ui->action9holes)
        this->setMode(Picaria::NineHoles);
    else if (action == ui->action13holes)
        this->setMode(Picaria::ThirteenHoles);
    else
        Q_UNREACHABLE();
}

void Picaria::showGameOver(Player player) {
    switch (player) {
        case Picaria::RedPlayer:
            QMessageBox::information(this, tr("Vencedor"), tr("Parabéns, o jogador vermelho venceu."));
            break;
        case Picaria::BluePlayer:
            QMessageBox::information(this, tr("Vencedor"), tr("Parabéns, o jogador azul venceu."));
            break;
        default:
            Q_UNREACHABLE();
    }
}

void Picaria::updateStatusBar() {
    QString player(m_player == Picaria::RedPlayer ? "vermelho" : "azul");
    QString phase(m_phase == Picaria::DropPhase ? "colocar" : "mover");

    ui->statusbar->showMessage(tr("Fase de %1: vez do jogador %2").arg(phase).arg(player));
}

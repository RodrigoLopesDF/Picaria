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
    QObject::connect(this, SIGNAL(gameOver(Player)), this, SLOT(showGameOver(Player)));
    QObject::connect(this, SIGNAL(gameOver(Player)), this, SLOT(reset()));

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

int Picaria::getId(Hole* hole){
    for(int i=0;i<13;i++)
        if(m_holes[i] == hole)
            return i;
    return -1;
}

int Picaria::getNeighborId(int id, Picaria::Direction direction){
    int matrix_ThirteenHoles[13][8] = {{-1,-1, 1, 3, 5,-1,-1,-1},
                                       {-1,-1, 2, 4, 6, 3, 0,-1},
                                       {-1,-1,-1,-1, 7, 4, 1,-1},

                                       {-1, 1,-1, 6,-1, 5,-1, 0},
                                       {-1, 2,-1, 7,-1, 6,-1, 1},

                                       { 0, 3, 6, 8,10,-1,-1,-1},
                                       { 1, 4, 7, 9,11, 8, 5, 3},
                                       { 2,-1,-1,-1,12, 9, 6, 4},

                                       {-1, 6,-1,11,-1,10,-1, 5},
                                       {-1, 7,-1,12,-1,11,-1, 6},

                                       { 5, 8,11,-1,-1,-1,-1,-1},
                                       { 6, 9,12,-1,-1,-1,10, 8},
                                       { 7,-1,-1,-1,-1,-1,11, 9}};

    int matrix_NineHoles[13][8] = {{-1,-1, 1, 6, 5,-1,-1,-1},
                                   {-1,-1, 2, 7, 6, 5, 0,-1},
                                   {-1,-1,-1,-1, 7, 6, 1,-1},

                                   {-1,-1,-1,-1,-1,-1,-1,-1},
                                   {-1,-1,-1,-1,-1,-1,-1,-1},

                                   { 0, 1, 6,11,10,-1,-1,-1},
                                   { 1, 2, 7,12,11,10, 5, 0},
                                   { 2,-1,-1,-1,12,11, 6, 1},

                                   {-1,-1,-1,-1,-1,-1,-1,-1},
                                   {-1,-1,-1,-1,-1,-1,-1,-1},

                                   { 5, 6,11,-1,-1,-1,-1,-1},
                                   { 6, 7,12,-1,-1,-1,10, 5},
                                   { 7,-1,-1,-1,-1,-1,11, 6}};

    if(m_mode == Picaria::ThirteenHoles)
        return matrix_ThirteenHoles[id][direction];
    else
        return matrix_NineHoles[id][direction];
}

Hole* Picaria::getNeighborHole(Hole* hole, Picaria::Direction direction){
    if (hole == nullptr)
        return nullptr;
    int index = getNeighborId(getId(hole), direction);
    if (index != -1) {
        return m_holes[index];
    } else {
        return nullptr;
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
                move(hole);
                break;
            default:
                Q_UNREACHABLE();
        }
}

bool Picaria::isGameOver(Hole* hole){
    bool v1 = confereVitoria(getNeighborHole(hole,Picaria::West),hole,getNeighborHole(hole,Picaria::East));
    bool v2 = confereVitoria(getNeighborHole(hole,Picaria::North),hole,getNeighborHole(hole,Picaria::South));
    bool v3 = confereVitoria(getNeighborHole(hole,Picaria::Northwest),hole,getNeighborHole(hole,Picaria::Southeast));
    bool v4 = confereVitoria(getNeighborHole(hole,Picaria::Southwest),hole,getNeighborHole(hole,Picaria::Northeast));

    bool v5 = confereVitoria(hole,getNeighborHole(hole,Picaria::North),getNeighborHole(getNeighborHole(hole,Picaria::North),Picaria::North));
    bool v6 = confereVitoria(hole,getNeighborHole(hole,Picaria::Northeast),getNeighborHole(getNeighborHole(hole,Picaria::Northeast),Picaria::Northeast));
    bool v7 = confereVitoria(hole,getNeighborHole(hole,Picaria::East),getNeighborHole(getNeighborHole(hole,Picaria::East),Picaria::East));
    bool v8 = confereVitoria(hole,getNeighborHole(hole,Picaria::Southeast),getNeighborHole(getNeighborHole(hole,Picaria::Southeast),Picaria::Southeast));

    bool v9 = confereVitoria(hole,getNeighborHole(hole,Picaria::South),getNeighborHole(getNeighborHole(hole,Picaria::South),Picaria::South));
    bool v10 = confereVitoria(hole,getNeighborHole(hole,Picaria::Southwest),getNeighborHole(getNeighborHole(hole,Picaria::Southwest),Picaria::Southwest));
    bool v11 = confereVitoria(hole,getNeighborHole(hole,Picaria::West),getNeighborHole(getNeighborHole(hole,Picaria::West),Picaria::West));
    bool v12 = confereVitoria(hole,getNeighborHole(hole,Picaria::Northwest),getNeighborHole(getNeighborHole(hole,Picaria::Northwest),Picaria::Northwest));

    return (v1 || v2 || v3 || v4 || v5 || v6 || v7 || v8 || v9 || v10 || v11 || v12);
}

bool Picaria::confereVitoria(Hole* hole1, Hole* hole2, Hole* hole3){
    if(hole1 != nullptr && hole2 != nullptr && hole3 != nullptr){
        Hole::State state = hole1->state();
        if(state == Hole::BlueState || state == Hole::RedState){
            return (hole2->state() == state && hole3->state() == state);
        }
    }
    return false;
}


void Picaria::move(Hole* hole){
    QPair<Hole*,Hole*>* movement = nullptr;
        if (hole->state() == Hole::SelectableState) {
            Q_ASSERT(m_selected != 0);
            movement = new QPair<Hole*,Hole*>(m_selected, hole);
        } else {
            if (hole->state() == player2state(m_player)) {
                QList<Hole*> selectable = this->findSelectable(hole);
                if (selectable.count() == 1) {
                    movement = new QPair<Hole*,Hole*>(hole, selectable.at(0));
                } else if (selectable.count() > 1) {
                    this->clearSelectable();
                    foreach (Hole* tmp, selectable)
                        tmp->setState(Hole::SelectableState);
                    m_selected = hole;
                }
            }
        }

        if (movement != nullptr) {
            this->clearSelectable();
            m_selected = 0;

            Q_ASSERT(movement->first->state() == player2state(m_player));
            Q_ASSERT(movement->second->state() == Hole::EmptyState);

            movement->first->setState(Hole::EmptyState);
            movement->second->setState(player2state(m_player));

            if (isGameOver(movement->second))
                emit gameOver(m_player);
            else
                this->switchPlayer();

            delete movement;
        }
    }

void Picaria::drop(Hole* hole) {
    if (hole->state() == Hole::EmptyState){
        hole->setState(player2state(m_player));

        if (isGameOver(hole))
            emit gameOver(m_player);
        else {
            ++m_dropCount;
            if (m_dropCount == 6)
                m_phase = Picaria::MovePhase;
            this->switchPlayer();
        }
    }
}


void Picaria::clearSelectable() {
    for (int id = 0; id < 13; id++) {
        Hole* hole = m_holes[id];
        if (hole->state() == Hole::SelectableState)
            hole->setState(Hole::EmptyState);
    }
}

bool isSelectable(Hole* hole) {
    return hole != nullptr &&
            (hole->state() == Hole::EmptyState ||
             hole->state() == Hole::SelectableState);
}

QList<Hole*> Picaria::findSelectable(Hole *hole) {
    QList<Hole*> list;

    Hole* north = getNeighborHole(hole,Picaria::North);
    if (isSelectable(north))
        list << north;

    Hole* northeast = getNeighborHole(hole,Picaria::Northeast);
    if (isSelectable(northeast))
        list << northeast;

    Hole* east = getNeighborHole(hole,Picaria::East);
    if (isSelectable(east))
        list << east;

    Hole* southeast = getNeighborHole(hole,Picaria::Southeast);
    if (isSelectable(southeast))
        list << southeast;

    Hole* south = getNeighborHole(hole,Picaria::South);
    if (isSelectable(south))
        list << south;

    Hole* southwest = getNeighborHole(hole,Picaria::Southwest);
    if (isSelectable(southwest))
        list << southwest;

    Hole* west = getNeighborHole(hole,Picaria::West);
    if (isSelectable(west))
        list << west;

    Hole* northwest = getNeighborHole(hole,Picaria::Northwest);
    if (isSelectable(northwest))
        list << northwest;

    return list;
}

void Picaria::updateMode(QAction* action) {
    if (action == ui->action9holes)
        this->setMode(Picaria::NineHoles);
    else if (action == ui->action13holes)
        this->setMode(Picaria::ThirteenHoles);
    else
        Q_UNREACHABLE();
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
    m_dropCount = 0;
    m_selected = nullptr;

    // Finally, update the status bar.
    this->updateStatusBar();
}

void Picaria::showAbout() {
    QMessageBox::information(this, tr("About"), tr("Picaria\nFeito por:\n - Rodrigo Lopes - rodrigolopesferreira4@gmail.com\n - Henrique Coelho - h8mendes@gmail.com"));
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

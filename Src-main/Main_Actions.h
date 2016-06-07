#ifndef MAIN_ACTIONS_H
#define MAIN_ACTIONS_H

#include <QObject>

class QAction;
class QMenu;
class QMainWindow;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Main console window menu items
//
class Main_Actions : public QObject
{
    Q_OBJECT

public:
    QAction
    // File
        *fileOpenAct,
        *newAcqAct,
        *stopAcqAct,
        *quitAct,
    // Options
        *togDebugAct,
        *selRunDirAct,
        *aoDlgAct,
        *cmdSrvOptAct,
        *rgtSrvOptAct,
    // Tools
        *imBistAct,
        *sha1Act,
        *par2Act,
        *editLogAct,
        *logFileAct,
    // Window
        *bringFrontAct,
        *shwHidConsAct,
        *shwHidGrfsAct,
    // Help
        *helpAct,
        *aboutAct,
        *aboutQtAct;

    QMenu   *windowMenu;

public:
    Main_Actions() : QObject(0) {}

    void initActions();
    void initMenus( QMainWindow *w );
};

#endif  // MAIN_ACTIONS_H



#ifndef SHANKEDITTAB_H
#define SHANKEDITTAB_H

#include "IMROTbl.h"

#include <QObject>

namespace Ui {
class ShankEditTab;
}

class ShankCtlBase;

class QLabel;
class QMenu;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankEditTab : public QObject
{
    Q_OBJECT

private:
    ShankCtlBase            *SC;
    Ui::ShankEditTab        *seTabUI;
    QMenu                   *mClear;
    IMROTbl                 *R0,
                            *Rfile,
                            *R;
    IMRO_GUI                G;
    std::vector<IMRO_Site>  vX;
    std::vector<IMRO_ROI>   vR;
    QString                 filename,
                            lastDir;
    bool                    canEdit;

public:
    ShankEditTab(
        ShankCtlBase    *SC,
        QWidget         *tab,
        const IMROTbl   *inR );
    virtual ~ShankEditTab();

    void hideCurrent();
    void setCurrent( const QString &curFile );
    void renameOriginal();
    void hideOKBut();
    void renameApplyRevert();
    void syncYPix( int y );
    void gridHover( int s, int r );
    void gridClicked( int s, int r, bool shift );
    void beep( const QString &msg );

signals:
    void imroChanged( QString newName );

private slots:
    void ypixChanged( int y );
    void loadBut();
    void defBut();
    void clearAll();
    void clearShank0();
    void clearShank1();
    void clearShank2();
    void clearShank3();
    bool saveBut();
    void helpBut();
    void okBut();
    void cancelBut();

private:
    void initItems();
    void initClearMenu();
    void initBoxes();
    void enableItems( bool enabled, bool clear = false );
    void clearShank( int s );
    void R2GUI();
    bool GUI2R();
    bool isDone();
    bool isDefault();
    int where( int &gap0, int &gapLim, int s, int r );
    int selected( int gap0, int gapLim, int code, int s );
    void clickHere( IMRO_ROI &C, int gap0, int gapLim, int s, int r );
    void buildTestBoxes( tImroROIs vT, int s );
    int shortfall( int s );
    void updateSums( int s );
    int getSum( int s );
    int getRqd( int s );
    QLabel *getSelObj( int s );
    QLabel *getSumObj( int s );
    QLabel *getRqdObj( int s );
    void color();
    void loadSettings();
    void saveSettings() const;
};

#endif  // SHANKEDITTAB_H



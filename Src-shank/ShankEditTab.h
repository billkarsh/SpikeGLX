#ifndef SHANKEDITTAB_H
#define SHANKEDITTAB_H

#include "IMROTbl.h"

#include <QObject>

namespace Ui {
class ShankEditTab;
}

class ShankCtlBase;
class ShankEditTab;

class QLabel;
class QMenu;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankEditTab : public QObject
{
    Q_OBJECT

private:
    struct Click {
        ShankEditTab    *se;
        int             s,
                        rclick,
                        code,
                        gap0,
                        gapLim,
                        edge0,
                        ib;     // -1 if no drag
        Click( ShankEditTab *se ) : se(se), ib(-1)    {}
        void down( int s, int r );
        void drag( int s, int r );
        void up();
    private:
        int where();
        void buildTestBoxes( tImroROIs vT );
        void newBox();
        int selectBox();
    };

private:
    ShankCtlBase            *SC;
    Ui::ShankEditTab        *seTabUI;
    QMenu                   *mClear;
    IMROTbl                 *R0,
                            *Rfile,
                            *R;
    Click                   click;
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
    void gridHover( int s, int r, bool quiet );
    void gridClicked( int s, int r, bool shift );
    void lbutReleased();
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
    bool isDefault();
    bool isDone();
    bool isFull( int s );
    void addBox( const IMRO_ROI &B, int s );
    void updateAll( int ib, int s );
    void updateSums( int s );
    int rowsShortfall( int s );
    int getSum( int s );
    int getRqd( int s );
    void setSel( int s, int ib );
    QLabel *getSumObj( int s );
    QLabel *getRqdObj( int s );
    void color();
    void loadSettings();
    void saveSettings() const;
};

#endif  // SHANKEDITTAB_H



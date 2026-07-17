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
                        sel_ib,     // -1 if no drag
                        sel_edge0,  // dragging-edge initial row
                        code,       // {-1,0,1,2} = {vX,new,lower,upper}
                        gap0,       // lower bound
                        gapLim;     // upper bound
        bool            isW;

        Click( ShankEditTab *se ) : se(se), sel_ib(-1), isW(false)  {}
        void down( int s, int c, int r, bool ctrl );
        void drag( int s, int r );
        void up();
    private:
        bool selected( int c, int r, bool testW );
        void sel_set_gap0( int sel_FLR );
        void sel_set_gapLim( int sel_FLR );
        void where( int c );
        void buildTestBoxes( tImroROIs vT, int click_FLR, int h );
        void newBox( int c );
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
    std::vector<IMRO_Site>  vX,
                            vI;
    std::vector<IMRO_ROI>   vR,
                            vW;
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
    void gridClicked( int s, int c, int r, bool shift, bool ctrl );
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
    void saveChansBut();
    bool saveBut();
    void helpBut();
    void okBut();
    void cancelBut();

private:
    int usageMode();
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
    void addBox( const IMRO_ROI &B );
    void addW( const IMRO_ROI &B );
    void updateAll( int s, int ib );
    void updateW( int s, int ib );
    void updateSums( int s );
    int rowsShortfall( const IMRO_ROI &B );
    int rowsShortfallW( const IMRO_ROI &B );
    int getSum( int s );
    int getSumW( int s );
    int getRqd();
    void setSel( int s, int ib, bool isW );
    QLabel *getSumObj( int s );
    void color();
    void loadSettings();
    void saveSettings() const;
};

#endif  // SHANKEDITTAB_H



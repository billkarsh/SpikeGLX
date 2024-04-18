#ifndef SHANKEDITTAB_H
#define SHANKEDITTAB_H

#include "IMROTbl.h"

#include <QObject>

namespace Ui {
class ShankEditTab;
}

class ShankCtlBase;

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
    std::vector<IMRO_Site>  vX;
    std::vector<IMRO_ROI>   vR;
    QString                 filename,
                            lastDir;
    int                     nBase,
                            grid,
                            nBoxes[4];
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
    void gridClicked( int s, int r );
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
    void bx0CBChanged();
    void bx1CBChanged();
    void bx2CBChanged();
    void bx3CBChanged();
    bool saveBut();
    void helpBut();
    void okBut();
    void cancelBut();

private:
    void initItems();
    void initClearMenu();
    void initBoxes();
    void enableItems( bool enabled, bool onebox = false );
    void clearShank( int s );
    int boxRows( int s );
    int boxesOnShank( int s );
    void bxCBChanged( int s );
    void R2GUI();
    bool GUI2R();
    bool isDefault();
    bool forbidden( int s, int r );
    void clickHere( IMRO_ROI &C, int s, int r );
    void buildTestBoxes( tImroROIs vT, int s );
    bool fitIntoGap( IMRO_ROI &C );
    bool boxOverlapsMe( const IMRO_ROI &Me, const IMRO_ROI &B );
    bool boxIsBelowMe( const IMRO_ROI &Me, const IMRO_ROI &B );
    void color();
    void loadSettings();
    void saveSettings() const;
};

#endif  // SHANKEDITTAB_H



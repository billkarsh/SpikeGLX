#ifndef SHANKEDITTAB_H
#define SHANKEDITTAB_H

#include "IMROTbl.h"

#include <QObject>

namespace Ui {
class ShankEditTab;
}

class ShankCtlBase;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankEditTab : public QObject
{
    Q_OBJECT

private:
    ShankCtlBase            *SC;
    Ui::ShankEditTab        *seTabUI;
    IMROTbl                 *R0,
                            *Rfile,
                            *R;
    std::vector<IMRO_Site>  vX;
    std::vector<IMRO_ROI>   vR;
    QString                 filename,
                            lastDir;
    int                     grid,
                            nBoxes,
                            boxRows;
    bool                    canEdit;

public:
    ShankEditTab(
        ShankCtlBase    *SC,
        QWidget         *tab,
        const IMROTbl   *inR );
    virtual ~ShankEditTab();

    void hideOKBut();
    void syncYPix( int y );
    void gridHover( int s, int c, int r );
    void gridClicked( int s, int c, int r );
    void beep( const QString &msg );

signals:
    void imroChanged( QString newName );

private slots:
    void loadBut();
    void defBut();
    void clearBut();
    void ypixChanged( int y );
    void bxCBChanged();
    void saveBut();
    void helpBut();
    void okBut();
    void cancelBut();

private:
    void initItems();
    void enableItems( bool enabled );
    void setBoxRows();
    void R2GUI();
    bool isDefault();
    bool forbidden( int s, int c, int r );
    void clickHere( IMRO_ROI &C, int s, int r );
    void buildTestBoxes( tImroROIs vT, int s );
    bool fitIntoGap( IMRO_ROI &C );
    bool hitBoxAbove( const IMRO_ROI &C, const IMRO_ROI &A );
    bool hitBoxBelow( const IMRO_ROI &C, const IMRO_ROI &B );
    void color();
    void loadSettings();
    void saveSettings() const;
};

#endif  // SHANKEDITTAB_H



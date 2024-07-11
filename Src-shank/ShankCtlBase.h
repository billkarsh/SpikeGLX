#ifndef SHANKCTLBASE_H
#define SHANKCTLBASE_H

#include "ShankEditTab.h"

#include <QDialog>
#include <QMutex>

namespace Ui {
class ShankWindow;
}

class ShankView;
class ShankScroll;
struct ShankMap;
class MGraphX;
class MGraphY;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankCtlBase : public QDialog
{
    Q_OBJECT

protected:
    Ui::ShankWindow     *scUI;
    ShankEditTab        *seTab;
    ShankMap            *modal_map;
    bool                modal;

public:
    mutable QMutex      drawMtx;

public:
    ShankCtlBase( QWidget *parent, bool modal );
    virtual ~ShankCtlBase();

    void baseInit( const IMROTbl *R, bool hasViewTab );
    void setOriginal( const QString &inFile );
    void showDialog();

    void syncYPix( int y );
    ShankView* view() const;
    ShankScroll* scroll() const;
    void setStatus( const QString &s );
    void update();

    virtual void setAnatomyPP( const QString& /*elems*/, int /*sk*/ )
        {}
    virtual void colorTraces( MGraphX* /*theX*/, std::vector<MGraphY>& /*vY*/ )
        {}

    virtual void loadSettings()         {}
    virtual void saveSettings() const   {}

signals:
    void runSaveChansDlg( QString sInit );
    void modal_done( ShankCtlBase *editor, QString fn, bool ok );
    void viewTabSyncYPix( int y );
    void feedMe( bool needMap );
    void gimmeTraces();

public slots:
    void gridHover( int s, int r, bool quiet );
    void gridClicked( int s, int c, int r, bool shift );
    void lbutReleased();

protected:
    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
};

#endif  // SHANKCTLBASE_H



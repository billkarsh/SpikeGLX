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
class ShankMap;

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

    virtual void loadSettings()         {}
    virtual void saveSettings() const   {}

signals:
    void modal_done( ShankCtlBase *editor, QString fn, bool ok );
    void viewTabSyncYPix( int y );
    void feedMe( bool needMap );

public slots:
    void gridHover( int s, int c, int r );
    void gridClicked( int s, int c, int r );

protected:
    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
};

#endif  // SHANKCTLBASE_H



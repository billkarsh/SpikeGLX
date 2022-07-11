#ifndef FVW_SHANKCTL_H
#define FVW_SHANKCTL_H

#include "ShankViewTab.h"

#include <QDialog>

namespace Ui {
class FVW_ShankWindow;
}

class ShankView;
class ShankScroll;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FVW_ShankCtl : public QDialog
{
    Q_OBJECT

    friend class ShankViewTab;

protected:
    const DataFile      *df;
    Ui::FVW_ShankWindow *scUI;
    ShankViewTab        *svTab;

public:
    FVW_ShankCtl( const DataFile *df, QWidget *parent = 0 );
    virtual ~FVW_ShankCtl();

    virtual void init( const ShankMap *map ) = 0;
    void showDialog();

    void mapChanged( const ShankMap *map )  {svTab->mapChanged( map );}
    void selChan( int ig )                  {svTab->selChan( ig );}

    void putInit()                          {svTab->putInit();}
    void putScans( const vec_i16 &_data )   {svTab->putScans( _data );}
    void putDone()                          {svTab->putDone();}

signals:
    void feedMe();
    void selChanged( int ig );
    void closed( QWidget *w );

public slots:
    void cursorOver( int ig );
    void lbutClicked( int ig );

protected:
    void baseInit( const ShankMap *map, int bnkRws = 0 );

    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

    virtual void loadSettings() = 0;
    virtual void saveSettings() const = 0;

    virtual QString screenStateName() const = 0;
    void restoreScreenState();
    void saveScreenState() const;

private:
    ShankView* view() const;
    ShankScroll* scroll() const;
    void setStatus( const QString &s );
};

#endif  // FVW_SHANKCTL_H



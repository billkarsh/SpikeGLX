#ifndef FVSHANKCTL_H
#define FVSHANKCTL_H

#include "ShankCtlBase.h"
#include "FVShankViewTab.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FVShankCtl : public ShankCtlBase
{
    Q_OBJECT

protected:
    const DataFile  *df;
    FVShankViewTab  *svTab;

public:
    FVShankCtl( const DataFile *df, QWidget *parent = 0 );
    virtual ~FVShankCtl();

    virtual void init( const ShankMap *map ) = 0;

    void mapChanged( const ShankMap *map )  {svTab->mapChanged( map );}
    void selChan( int sh, int bk, int ig )  {svTab->selChan( sh, bk, ig );}

    void putInit()                          {svTab->putInit();}
    void putScans( const vec_i16 &_data )   {svTab->putScans( _data );}
    void putDone()                          {svTab->putDone();}

signals:
    void selChanged( int ig );
    void closed( QWidget *w );

public slots:
    void cursorOver( int ig )               {svTab->cursorOver( ig );}
    void lbutClicked( int ig );

protected:
    void parInit( const ShankMap *map, int bnkRws = 0 );
    virtual void closeEvent( QCloseEvent *e );

    virtual QString screenStateName() const = 0;
    void restoreScreenState();
    void saveScreenState() const;
};

#endif  // FVSHANKCTL_H



#ifndef FVSHANKCTL_H
#define FVSHANKCTL_H

#include "ShankCtlBase.h"
#include "FVShankViewTab.h"

class QFile;

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
    void putSamps( const vec_i16 &_data )   {svTab->putSamps( _data );}
    void putDone()                          {svTab->putDone();}

    virtual void exportHeat( QFile *f, const double *val ) = 0;

signals:
    void selChanged( int ig );
    void feedMe( bool needMap );

public slots:
    void cursorOver( int ig )               {svTab->cursorOver( ig );}
    void lbutClicked( int ig );

protected:
    void parInit( const ShankMap *map );
    virtual void closeEvent( QCloseEvent *e );
};

#endif  // FVSHANKCTL_H



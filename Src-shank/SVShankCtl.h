#ifndef SVSHANKCTL_H
#define SVSHANKCTL_H

#include "ShankCtlBase.h"
#include "SVShankViewTab.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVShankCtl : public ShankCtlBase
{
    Q_OBJECT

protected:
    const DAQ::Params   &p;
    SVShankViewTab      *svTab;
    QAction             *audioLAction,
                        *audioBAction,
                        *audioRAction,
                        *spike1Action,
                        *spike2Action,
                        *spike3Action,
                        *spike4Action;
    int                 jpanel;

public:
    SVShankCtl( const DAQ::Params &p, int jpanel, QWidget *parent = 0 );
    virtual ~SVShankCtl();

    virtual void init() = 0;
    virtual void mapChanged() = 0;
    void selChan( int ic, const QString &name )
        {svTab->selChan( ic, name );}

    void putSamps( const vec_i16 &_data, quint64 headCt )
        {svTab->putSamps( _data, headCt );}

signals:
    void selChanged( int ic, bool shift );
    void closed( QWidget *w );

public slots:
    virtual void cursorOver( int ic, bool shift ) = 0;
    virtual void lbutClicked( int ic, bool shift ) = 0;

protected:
    void parInit( int js, int ip );
    virtual void closeEvent( QCloseEvent *e );
};

#endif  // SVSHANKCTL_H



#ifndef SHANKCTL_IM_H
#define SHANKCTL_IM_H

#include "ShankCtl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankCtl_Im : public ShankCtl
{
    Q_OBJECT

public:
    ShankCtl_Im( DAQ::Params &p, QWidget *parent = 0 );
    virtual ~ShankCtl_Im()  {saveSettings();}

    virtual void init();

    virtual void putScans( const vec_i16 &_data );

public slots:
    virtual void cursorOver( int ic, bool shift );
    virtual void lbutClicked( int ic, bool shift );

protected:
    virtual void updateFilter( bool lock );

    virtual void loadSettings();
    virtual void saveSettings() const;
};

#endif  // SHANKCTL_IM_H



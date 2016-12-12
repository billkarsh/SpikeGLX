#ifndef SHANKCTL_NI_H
#define SHANKCTL_NI_H

#include "ShankCtl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankCtl_Ni : public ShankCtl
{
    Q_OBJECT

public:
    ShankCtl_Ni( DAQ::Params &p, QWidget *parent = 0 );
    virtual ~ShankCtl_Ni()  {saveSettings();}

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

#endif  // SHANKCTL_NI_H



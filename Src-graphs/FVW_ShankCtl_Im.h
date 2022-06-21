#ifndef FVW_SHANKCTL_IM_H
#define FVW_SHANKCTL_IM_H

#include "FVW_ShankCtl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FVW_ShankCtl_Im : public FVW_ShankCtl
{
    Q_OBJECT

public:
    FVW_ShankCtl_Im( const DataFile *df, QWidget *parent = 0 );
    virtual ~FVW_ShankCtl_Im()  {saveScreenState();}

    virtual void init( const ShankMap *map );

protected:
    QString settingsName() const;
    virtual void loadSettings();
    virtual void saveSettings() const;

    virtual QString screenStateName() const;
};

#endif  // FVW_SHANKCTL_IM_H



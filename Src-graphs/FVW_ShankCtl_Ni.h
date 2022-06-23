#ifndef FVW_SHANKCTL_NI_H
#define FVW_SHANKCTL_NI_H

#include "FVW_ShankCtl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FVW_ShankCtl_Ni : public FVW_ShankCtl
{
    Q_OBJECT

public:
    FVW_ShankCtl_Ni( const DataFile *df, QWidget *parent = 0 );
    virtual void init( const ShankMap *map );

protected:
    QString settingsName() const;
    virtual void loadSettings();
    virtual void saveSettings() const;

    virtual QString screenStateName() const;
};

#endif  // FVW_SHANKCTL_NI_H



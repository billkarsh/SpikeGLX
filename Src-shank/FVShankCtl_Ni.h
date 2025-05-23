#ifndef FVSHANKCTL_NI_H
#define FVSHANKCTL_NI_H

#include "FVShankCtl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FVShankCtl_Ni : public FVShankCtl
{
    Q_OBJECT

public:
    FVShankCtl_Ni( const DataFile *df, QWidget *parent = 0 );
    virtual void init( const ShankMap *map );
    virtual void exportHeat( QFile *f, const double *val );

protected:
    QString settingsName() const;
    virtual void loadSettings();
    virtual void saveSettings() const;

    virtual QString screenStateName() const;
};

#endif  // FVSHANKCTL_NI_H



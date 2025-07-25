#ifndef FVSHANKCTL_IM_H
#define FVSHANKCTL_IM_H

#include "FVShankCtl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FVShankCtl_Im : public FVShankCtl
{
    Q_OBJECT

public:
    FVShankCtl_Im( const DataFile *df, QWidget *parent = 0 );
    virtual void init( const ShankMap *map );
    virtual int fvw_maxr() const;
    virtual void setAnatomyPP( const QString &elems, int sk );
    virtual void colorTraces( MGraphX *theX, std::vector<MGraphY> &vY );
    virtual QString getLbl( int s, int r ) const;
    virtual void exportHeat( QFile *f, const double *val );

protected:
    QString settingsName() const;
    virtual void loadSettings();
    virtual void saveSettings() const;

    virtual QString screenStateName() const;
};

#endif  // FVSHANKCTL_IM_H



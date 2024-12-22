#ifndef SVSHANKCTL_NI_H
#define SVSHANKCTL_NI_H

#include "SVShankCtl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVShankCtl_Ni : public SVShankCtl
{
    Q_OBJECT

public:
    SVShankCtl_Ni(
        const DAQ::Params   &p,
        int                 jpanel,
        QWidget             *parent = 0 );
    virtual void init();
    virtual void mapChanged();

public slots:
    virtual void cursorOver( int ic, bool shift );
    virtual void lbutClicked( int ic, bool shift );

protected:
    QString settingsName() const;
    virtual void loadSettings();
    virtual void saveSettings() const;

    virtual QString screenStateName() const;
};

#endif  // SVSHANKCTL_NI_H



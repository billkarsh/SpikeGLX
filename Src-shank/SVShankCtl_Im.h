#ifndef SVSHANKCTL_IM_H
#define SVSHANKCTL_IM_H

#include "SVShankCtl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVShankCtl_Im : public SVShankCtl
{
    Q_OBJECT

private:
    int ip;

public:
    SVShankCtl_Im(
        const DAQ::Params   &p,
        int                 ip,
        int                 jpanel,
        QWidget             *parent = 0 );
    virtual void init();
    virtual void mapChanged();
    virtual void setAnatomyPP( const QString &elems, int sk );

public slots:
    virtual void cursorOver( int ic, bool shift );
    virtual void lbutClicked( int ic, bool shift );

private slots:
    void imroChanged( QString newName );

protected:
    QString settingsName() const;
    virtual void loadSettings();
    virtual void saveSettings() const;
};

#endif  // SVSHANKCTL_IM_H



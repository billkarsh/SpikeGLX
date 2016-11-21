#ifndef SHANKCTL_NI_H
#define SHANKCTL_NI_H

#include "ShankCtl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankCtl_Ni : public ShankCtl
{
    Q_OBJECT

private:

public:
    ShankCtl_Ni( DAQ::Params &p, QWidget *parent = 0 );
    virtual ~ShankCtl_Ni();

    virtual void init();

public slots:

private slots:
    void cursorOver( int ic );
    void lbutClicked( int ic );

protected:
    virtual void loadSettings();
    virtual void saveSettings() const;

private:
};

#endif  // SHANKCTL_NI_H



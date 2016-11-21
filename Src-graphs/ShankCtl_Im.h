#ifndef SHANKCTL_IM_H
#define SHANKCTL_IM_H

#include "ShankCtl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankCtl_Im : public ShankCtl
{
    Q_OBJECT

private:

public:
    ShankCtl_Im( DAQ::Params &p, QWidget *parent = 0 );
    virtual ~ShankCtl_Im();

    virtual void init();

public slots:

private slots:
    void cursorOver( int ic, bool shift );
    void lbutClicked( int ic, bool shift );

protected:
    virtual void loadSettings();
    virtual void saveSettings() const;

private:
};

#endif  // SHANKCTL_IM_H



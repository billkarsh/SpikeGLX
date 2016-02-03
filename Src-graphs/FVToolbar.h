#ifndef FVTOOLBAR_H
#define FVTOOLBAR_H

#include <QToolBar>

class FileViewerWindow;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FVToolbar : public QToolBar
{
    Q_OBJECT

private:
    FileViewerWindow    *fv;

public:
    FVToolbar( FileViewerWindow *fv );

    void setRanges();

    void setSortButText( const QString &name );
    void setSelName( const QString &name );
    void enableYPix( bool enabled );
    void setYSclAndGain( double &yScl, double &gain, bool enabled );
    void setFltChecks( bool hp, bool dc, bool enabled );
    void setNDivText( const QString &s );
};

#endif  // FVTOOLBAR_H



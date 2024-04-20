#ifndef FVTOOLBAR_H
#define FVTOOLBAR_H

#include <QToolBar>

class FileViewerWindow;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Toolbar for FV:
//
// Srt Shk Sel | [scs^] | [YPx^] [Yscl^] [Gan^] | NDiv Bxs | [band^] -<Tn> -<S> [BM^] | -<Tx> | All
//
class FVToolbar : public QToolBar
{
    Q_OBJECT

private:
    FileViewerWindow    *fv;

public:
    FVToolbar( FileViewerWindow *fv, int fType );

    void disableSorting();
    void setSortButText( const QString &name );
    void setSelName( const QString &name );
    void setXScale( double secs );
    void enableYPix( bool enabled );
    void setYSclAndGain( double yScl, double gain, bool enabled );
    void setNDivText( const QString &s );
};

#endif  // FVTOOLBAR_H



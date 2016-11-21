#ifndef MNAVBAR_H
#define MNAVBAR_H

#include <QToolBar>

class SVGrafsM;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Toolbar for SVGrafsM:
//
// Sort Shnk [NChn^] [1st^] [---|-----]
//
class MNavbar : public QToolBar
{
    Q_OBJECT

private:
    SVGrafsM    *gr;

public:
    MNavbar( SVGrafsM *gr );

    void setEnabled( bool enabled );
    void setPage( int page );
    int page() const;
    int first() const;
    void update();

private slots:
    void nchanChanged( int val, bool notify = true );
    void pageChanged( int val, bool notify = true );
};

#endif  // MNAVBAR_H



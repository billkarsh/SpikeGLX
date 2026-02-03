#ifndef CALSRATECTL_H
#define CALSRATECTL_H

#include "CalSRate.h"

#include <QDialog>

namespace Ui {
class CalSRateDlg;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class CalSRateCtl : public QDialog
{
    Q_OBJECT

private:
    Ui::CalSRateDlg             *calUI;
    DFRunTag                    runTag;
    std::vector<CalSRStream>    vIM;
    std::vector<CalSRStream>    vOB;
    std::vector<CalSRStream>    vNI;
    CalSRThread                 *thd;

public:
    CalSRateCtl();
    virtual ~CalSRateCtl();

public slots:
    void percent( int pct );
    void finished();

private slots:
    void browse();
    void go();
    void helpBut();
    void apply();

private:
    void write( const QString &s );
    bool verifySelection( QString &f );
    void setJobsOne( QString &f );
    void setJobsAll( QString &f );
};

#endif  // CALSRATECTL_H



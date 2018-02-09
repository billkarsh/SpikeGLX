#ifndef CALSRATECTL_H
#define CALSRATECTL_H

#include "CalSRate.h"

namespace Ui {
class CalSRateDlg;
}

class HelpButDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class CalSRateCtl : public QObject
{
    Q_OBJECT

private:
    HelpButDialog           *dlg;
    Ui::CalSRateDlg         *calUI;
    QString                 baseName;
    QVector<CalSRStream>    vIM;
    QVector<CalSRStream>    vNI;
    CalSRThread             *thd;

public:
    CalSRateCtl( QObject *parent = 0 );
    virtual ~CalSRateCtl();

public slots:
    void percent( int pct );
    void finished();

private slots:
    void browse();
    void go();
    void apply();

private:
    void write( const QString &s );
    bool verifySelection( QString &f );
    void setJobsOne( QString &f );
    void setJobsAll( QString &f );
};

#endif  // CALSRATECTL_H



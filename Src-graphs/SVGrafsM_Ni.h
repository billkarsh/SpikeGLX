#ifndef SVGRAFSM_NI_H
#define SVGRAFSM_NI_H

#include "SVGrafsM.h"

class Biquad;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVGrafsM_Ni : public SVGrafsM
{
    Q_OBJECT

private:
    QVector<float>  dcLvl,
                    dcSum;
    QVector<int>    dcCnt;
    double          dcClock;
    Biquad          *hipass,
                    *lopass;
    mutable QMutex  fltMtx;

public:
    SVGrafsM_Ni( GraphsWindow *gw, DAQ::Params &p );
    virtual ~SVGrafsM_Ni();

    virtual void putScans( vec_i16 &data, quint64 headCt );

    virtual int chanCount() const;
    virtual bool isBandpass() const         {return true;}
    virtual QString filterChkTitle() const  {return QString::null;}
    virtual bool isSelAnalog() const;
    virtual void setRecordingEnabled( bool )    {}

public slots:
    virtual void bandSelChanged( int sel );
    virtual void filterChkClicked( bool )       {}
    virtual void dcChkClicked( bool checked );
    virtual void binMaxChkClicked( bool checked );

private slots:
    virtual void mySaveGraphClicked( bool checked );

    virtual void myMouseOverGraph( double x, double y, int iy );
    virtual void myClickGraph( double x, double y, int iy );
    virtual void myRClickGraph( double, double, int )    {}

protected:
    virtual void myInit()   {}
    virtual double mySampRate();
    virtual void mySort_ig2ic();
    virtual QString myChanName( int ic ) const;
    virtual QBitArray& mySaveBits();
    virtual int mySetUsrTypes();

    virtual void saveSettings();
    virtual void loadSettings();

private:
    double scalePlotValue( double v, double gain );
    void computeGraphMouseOverVars(
        int         ic,
        double      &y,
        double      &mean,
        double      &stdev,
        double      &rms,
        const char* &unit );
};

#endif  // SVGRAFSM_NI_H



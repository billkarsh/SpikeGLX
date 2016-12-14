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
    Biquad          *hipass,
                    *lopass;
    mutable QMutex  fltMtx;

public:
    SVGrafsM_Ni( GraphsWindow *gw, const DAQ::Params &p );
    virtual ~SVGrafsM_Ni();

    virtual void putScans( vec_i16 &data, quint64 headCt );

    virtual int chanCount() const;
    virtual int neurChanCount() const;
    virtual bool isBandpass() const         {return true;}
    virtual QString filterChkTitle() const  {return QString::null;}
    virtual bool isSelAnalog() const;
    virtual void setRecordingEnabled( bool )    {}

public slots:
    virtual void bandSelChanged( int sel );
    virtual void filterChkClicked( bool )       {}
    virtual void sAveRadChanged( int radius );

private slots:
    virtual void mySaveGraphClicked( bool checked );

    virtual void myMouseOverGraph( double x, double y, int iy );
    virtual void myClickGraph( double x, double y, int iy );
    virtual void myRClickGraph( double, double, int )    {}

    void externSelectChan( int ic );

protected:
    virtual void myInit()   {}
    virtual double mySampRate() const;
    virtual void mySort_ig2ic();
    virtual QString myChanName( int ic ) const;
    virtual const QBitArray& mySaveBits() const;
    virtual int mySetUsrTypes();

    virtual void loadSettings();
    virtual void saveSettings() const;

private:
    double scalePlotValue( double v, double gain ) const;
    void computeGraphMouseOverVars(
        int         ic,
        double      &y,
        double      &mean,
        double      &stdev,
        double      &rms,
        const char* &unit ) const;
};

#endif  // SVGRAFSM_NI_H



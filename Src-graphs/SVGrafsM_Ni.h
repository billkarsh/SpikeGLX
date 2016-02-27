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
    Biquad          *hipass;
    mutable QMutex  hipassMtx;

public:
    SVGrafsM_Ni( GraphsWindow *gw, DAQ::Params &p );
    virtual ~SVGrafsM_Ni();

    virtual void putScans( vec_i16 &data, quint64 headCt );

    virtual int chanCount() const;
    virtual QString filterChkTitle() const  {return "300Hz MN hipass";}
    virtual bool isSelAnalog() const;

public slots:
    virtual void filterChkClicked( bool checked );

private slots:
    virtual void mySaveGraphClicked( bool checked );

    virtual void myMouseOverGraph( double x, double y, int iy );
    virtual void myClickGraph( double x, double y, int iy );

protected:
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



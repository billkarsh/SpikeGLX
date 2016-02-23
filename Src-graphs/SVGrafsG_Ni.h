#ifndef SVGRAFSG_NI_H
#define SVGRAFSG_NI_H

#include "SVGrafsG.h"

class Biquad;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVGrafsG_Ni : public SVGrafsG
{
    Q_OBJECT

private:
    Biquad          *hipass;
    mutable QMutex  hipassMtx;

public:
    SVGrafsG_Ni( GraphsWindow *gw, DAQ::Params &p );
    virtual ~SVGrafsG_Ni();

    virtual void putScans( vec_i16 &data, quint64 headCt );

    virtual bool isSelAnalog() const;

public slots:
    virtual void hipassClicked( bool checked );

private slots:
    virtual void mySaveGraphClicked( bool checked );

    virtual void myMouseOverGraph( double x, double y );
    virtual void myClickGraph( double x, double y );

protected:
    virtual int myChanCount();
    virtual double mySampRate();
    virtual void mySort_ig2ic();
    virtual int myGrfPerTab() const;
    virtual QString myChanName( int ic ) const;
    virtual QBitArray& mySaveBits();
    virtual void myCustomXSettings( int ic );

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

#endif  // SVGRAFSG_NI_H



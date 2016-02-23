#ifndef SVGRAFSG_IM_H
#define SVGRAFSG_IM_H

#include "SVGrafsG.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVGrafsG_Im : public SVGrafsG
{
    Q_OBJECT

private:
    bool    hipass;

public:
    SVGrafsG_Im( GraphsWindow *gw, DAQ::Params &p );
    virtual ~SVGrafsG_Im();

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

#endif  // SVGRAFSG_IM_H



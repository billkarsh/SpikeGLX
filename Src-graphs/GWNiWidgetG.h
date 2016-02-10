#ifndef GWNIWIDGETG_H
#define GWNIWIDGETG_H

#include "GWWidgetG.h"

class Biquad;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWNiWidgetG : public GWWidgetG
{
    Q_OBJECT

private:
    Biquad          *hipass;
    mutable QMutex  hipassMtx;

public:
    GWNiWidgetG( GraphsWindow *gw, DAQ::Params &p );
    virtual ~GWNiWidgetG();

    virtual void putScans( vec_i16 &data, quint64 headCt );

    virtual bool isChanAnalog( int ic ) const;
    virtual void hipassChecked( bool checked );

private slots:
    virtual void saveGraphClicked( bool checked );

    virtual void mouseOverGraph( double x, double y );
    virtual void mouseClickGraph( double x, double y );

protected:
    virtual int myChanCount();
    virtual void sort_ig2ic();
    virtual int getNumGraphsPerTab() const;
    virtual QString chanName( int ic ) const;
    virtual bool indexRangeThisType( int &c0, int &cLim, int ic );
    virtual QBitArray& mySaveBits();
    virtual void customXSettings( int ic );
    virtual QString settingsGrpName()   {return "PlotOptions_nidq";}

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

#endif  // GWNIWIDGETG_H



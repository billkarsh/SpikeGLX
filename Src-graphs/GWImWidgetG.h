#ifndef GWIMWIDGETG_H
#define GWIMWIDGETG_H

#include "GWWidgetG.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWImWidgetG : public GWWidgetG
{
    Q_OBJECT

private:
    bool    hipass;

public:
    GWImWidgetG( GraphsWindow *gw, DAQ::Params &p );
    virtual ~GWImWidgetG();

    virtual void putScans( vec_i16 &scans, quint64 firstSamp );

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
    virtual QString settingsGrpName()   {return "PlotOptions_imec";}

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

#endif  // GWIMWIDGETG_H



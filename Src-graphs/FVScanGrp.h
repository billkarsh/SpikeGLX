#ifndef FVSCANGRP_H
#define FVSCANGRP_H

#include <QWidget>

class FileViewerWindow;

class QSlider;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// The scan group is a trio of linked controls, with range texts:
//
// [pos^] to X (of Y) [secs^] to X (of Y) [---|------]
//
// The pos spinner allows single scan advance.
// The sec spinner allows millisecond advance.
// The slider allows chunked advance depending on pscale factor.
//
// User changes to one must update the other two, and any change
// requires updating the texts.
//
class FVScanGrp : public QWidget
{
    Q_OBJECT

private:
    FileViewerWindow    *fv;
    QSlider             *slider;
    qint64              pos,    // range [0..9E18]
                        pscale; // QSlider scaling factor

public:
    FVScanGrp( FileViewerWindow *fv );

    void setRanges( bool newFile );

    const QObject* getSliderObj() const {return (QObject*)slider;}
    qint64 getPos() const               {return pos;}
    double getTime() const              {return timeFromPos( pos );}

    double timeFromPos( qint64 p ) const;
    qint64 posFromTime( double s ) const;
    qint64 maxPos() const;
    void setFilePos64( qint64 newPos );
    void guiSetPos( qint64 newPos );

private slots:
    void posSBChanged( double p );
    void secSBChanged( double s );
    void sliderChanged( int i );

private:
    void updateTexts();
};

#endif  // FVSCANGRP_H



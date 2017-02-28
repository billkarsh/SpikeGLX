#ifndef FVSCANGRP_H
#define FVSCANGRP_H

#include <QWidget>

class FileViewerWindow;

class QSlider;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// The scan group is a duo of linked controls, with range text:
//
// [secs^] to X (of Y) [---|------] Update
//
// The sec spinner allows millisecond advance.
// The slider allows chunked advance depending on pscale factor.
//
// User changes to one must update the other, and any change
// requires updating the text.
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
    qint64 curPos() const               {return pos;}
    double curTime() const              {return timeFromPos( pos );}

    double timeFromPos( qint64 p ) const;
    qint64 posFromTime( double s ) const;
    qint64 maxPos() const;
    void setFilePos64( qint64 newPos );
    bool guiSetPos( qint64 newPos );
    void enableManualUpdate( bool enable );

private slots:
    void secSBChanged( double s );
    void sliderChanged( int i );
    void manualUpdateClicked();

private:
    void updateText();
};

#endif  // FVSCANGRP_H



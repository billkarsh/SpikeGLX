
#include "MainApp.h"
#include "KVParams.h"
#include "FileViewerWindow.h"

#include <QProcess>
#include <QSurfaceFormat>




int main( int argc, char *argv[] )
{
    qRegisterMetaType<KeyValMap>("KeyValMap");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<FileViewerWindow*>("FileViewerWindow*");

    QSurfaceFormat::setDefaultFormat( QSurfaceFormat::defaultFormat() );

    MainApp app( argc, argv );
    app.setStyle( "fusion" );

    return app.exec();
}



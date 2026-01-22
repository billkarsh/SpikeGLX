
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

    // These lines remind me what's available to force use of opengl
    // or other rendering options in similar manner. However, these
    // have no current role.
    //
    // qputenv( "QT_RHI_BACKEND", "opengl" );
    // QCoreApplication::setAttribute( Qt::AA_UseDesktopOpenGL );

    QSurfaceFormat::setDefaultFormat( QSurfaceFormat::defaultFormat() );

    MainApp app( argc, argv );
    app.setStyle( "fusion" );

    return app.exec();
}




#include "MainApp.h"
#include "KVParams.h"
#include "FileViewerWindow.h"

#include <QProcess>




int main( int argc, char *argv[] )
{
    qRegisterMetaType<KeyValMap>("KeyValMap");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<FileViewerWindow*>("FileViewerWindow*");

    MainApp app( argc, argv );

    return app.exec();
}



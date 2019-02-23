#ifndef DFNAME_H
#define DFNAME_H

#include <QFileInfo>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class DFRunTag
{
public:
    QString runDir,         // path/run_gN/ (term slash)
            runName;        // run          (undecorated)
    int     g, t;
    bool    fldPerPrb;
public:
    DFRunTag() : g(-1), t(-1), fldPerPrb(false) {}
    DFRunTag( const QString &dataDir, const QString &runName );
    DFRunTag( const QString &filePath );
    QString run_g_t() const;
    QString brevname( int ip, const QString &suffix ) const;
    QString filename( int ip, const QString &suffix ) const;
    bool operator==( const DFRunTag &rhs ) const
        {return t==rhs.t && runDir==rhs.runDir;}
};


class DFName
{
public:
// ------
// Static
// ------

    static QString justExtension( const QString &name );
    static QString chopExtension( const QString &name );
    static QString chopType( const QString &name );
    static QString forceBinSuffix( const QString &name );
    static QString forceMetaSuffix( const QString &name );

    static int typeAndIP( int &ip, const QString &name, QString *error );

    static bool isFileReadable( const QFileInfo &fi, QString *error );
    static qint64 fileSize( const QFileInfo &fi, QString *error );

    static bool isValidInputFile(
        const QString   &name,
        QString         *error,
        const QString   &reqVers = "20160120" );
};

#endif  // DFNAME_H



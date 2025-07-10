#ifndef KVPARAMS_H
#define KVPARAMS_H

#include <QMap>
#include <QString>
#include <QVariant>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

typedef QMap<QString,QVariant>  KeyValMap;


// KVParams class interconverts inifile-type data
// ('key=value' pairs) between an in-memory QMap
// and QString or disk-file versions.
//
class KVParams : public KeyValMap
{
public:
    KVParams() : QMap()                         {}
    KVParams( const KVParams &rhs ) : QMap(rhs) {}
    KVParams( const QString &s ) : QMap()       {fromString( s );}
    virtual ~KVParams()                         {}

    bool parseOneLine( QString &line );

    bool fromString( const QString &s );
    QString toString() const;

    bool fromMetaFile( const QString &metaFile );
    bool toMetaFile( const QString &metaFile ) const;
};

#endif  // KVPARAMS_H



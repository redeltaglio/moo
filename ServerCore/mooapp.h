#ifndef MOOAPP_H
#define MOOAPP_H

#include <QList>
#include <QMutex>
#include <QObject>

#include <lua.hpp>

#include "taskentry.h"

class mooApp : public QObject
{
    Q_OBJECT

public:
	explicit mooApp( const QString &pDataFileName = "moo.dat", QObject *pParent = 0 );

	virtual ~mooApp();

signals:
	void textOutput( const QString &pText );

public slots:
	void doTask( TaskEntry &pTask );
	void cleanup( QObject *pObject = 0 );

private slots:
	void doOutput( const QString &pText );

private:
	int					 mTimerId;
	const QString		 mDataFileName;

	static void streamCallback( const QString &pText, void *pUserData );

	void timerEvent( QTimerEvent *pEvent );
};

#endif // MOOAPP_H

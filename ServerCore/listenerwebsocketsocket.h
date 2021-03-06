#ifndef LISTENERWEBSOCKETSOCKET_H
#define LISTENERWEBSOCKETSOCKET_H

#include <QWebSocket>
#include <QTimer>

#include "listenersocket.h"

class ListenerWebSocketSocket : public ListenerSocket
{
	Q_OBJECT

public:
	ListenerWebSocketSocket( QObject *pParent, QWebSocket *pSocket );

	virtual ~ListenerWebSocketSocket( void ) {}

private slots:
	void disconnected( void );
	void textInput( const QString &pText );
	void inputTimeout( void );

//	void binaryFrameReceived(const QByteArray &frame, bool isLastFrame);
	void binaryMessageReceived(const QByteArray &message);

//	void textFrameReceived(const QString &frame, bool isLastFrame);
	void textMessageReceived(const QString &message);

signals:
	void textOutput( const QString &pText );

private:
	QWebSocket					*mSocket;
	QTimer						 mTimer;
};

#endif // LISTENERWEBSOCKETSOCKET_H

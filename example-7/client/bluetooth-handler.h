
#ifndef	__BLUETOOTH_HANDLER__
#define	__BLUETOOTH_HANDLER__
#include	<QThread>
#include	<QString>
#include	<QStringList>
#include        "protocol.h"
#include	"bluetooth.h"

class bluetoothHandler: public QThread {
Q_OBJECT
public:
		bluetoothHandler	(void);
		~bluetoothHandler	(void);
	void	write			(char *, int);
private:
virtual
	void	run			(void);
signals:
	void	ensembleLabel		(const QString &);
	void	serviceName		(const QString &);
	void	textMessage		(const QString &);
	void	programData		(const QString &);
	void	connectionLost		(void);
};

#endif


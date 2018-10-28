
#include	"bluetooth-handler.h"

	bluetoothHandler::bluetoothHandler (void) {
	  if (!bluetooth_connect ())
	     throw (21);
	   start ();
}

	bluetoothHandler::~bluetoothHandler (void) {
	bluetooth_terminate ();
}

void	bluetoothHandler::write (char *buf, int size) {
	bluetooth_write (buf, size);
}

void	bluetoothHandler::run	(void) {
	while (true) {
	   int bytes_read;
	   char buf [1024];
	   bytes_read = bluetooth_read (buf, sizeof (buf));
	   if (bytes_read < 0) {
	      emit connectionLost ();
	      break;
	   }

	   switch (buf [0]) {
	      case Q_ENSEMBLE:
	         ensembleLabel (QString (&buf [3]));
	         break;
	      case Q_SERVICE_NAME:
	         serviceName (QString (&buf [3]));
	         break;
	      case Q_TEXT_MESSAGE:
	         textMessage (QString (&buf [3]));
	         break;
	      case Q_PROGRAM_DATA:
	         programData (QString (&buf [3]));
	         break;
	      default:
	         fprintf (stderr, "ontvangen %\n", &buf [3]);
	   }
	}
}


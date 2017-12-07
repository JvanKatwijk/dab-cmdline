

// Variables for writing a server. 
/*
 *	1. Getting the address data structure.
 *	2. Opening a new socket.
 *	3. Bind to the socket.
 *	4. Listen to the socket. 
 *	5. Accept Connection.
 *	6. Receive Data.
 *	7. Close Connection. 
 *
 *	The controller "listens" to the port as given and
 *	eecutes the commands for setting the radio
 */
#define	BUF_SIZE	1024
void	controller (void) {
	struct sockaddr_in server;
	struct sockaddr_in client;
	int	socket_desc;
	int	client_sock;
	int	c;

	socket_desc = socket (AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
	   fprintf (stderr, "Could not create socket");
	   return;
	}
	fprintf (stderr, "Socket created");
//	Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons (port);
	
//	Bind
	if (bind (socket_desc,
	          (struct sockaddr *)&server , sizeof(server)) < 0) {
//	print the error message
	   perror ("bind failed. Error");
	   return;
	}

	fprintf (stderr, "now accepting connections ...\n");
	listen (socket_desc , 3);
	while (running. load ()) {
//	Accept a new connection and return back the socket desciptor 
	   c = sizeof (struct sockaddr_in);
     
//	accept connection from an incoming client
	   client_sock = accept (socket_desc, 
	                         (struct sockaddr *)&client,
	                         (socklen_t*)&c);
	   if (client_sock < 0) {
	      perror("accept failed");
	      return;
	   }
	   fprintf (stderr, "Connection accepted");
	   try {
	      uint8_t	localBuffer [BUF_SIZE];
	      while (running. load ()) {
	         int amount = recv (client_sock, localBuffer, amount ,0);
	         if (amount == -1) {
	            throw (22);
	         }
	         handleRequest (localBuffer, amount);
	      }
	   }
	   catch (int e) {
	       fprintf (stderr, "disconnected\n");
	   }
	}
	close (socket_desc);	
}

void	Request (uint8_t *buffer, int amount) {
int	value;

	if (amount < 2)
	   return;

	switch (buffer [0]) {
	   case Q_QUIT:
	      throw (33);
	      break;

	   case Q_GAIN:
	      my_radioData -> theGain = stoi (std::string ((char *)(&buffer [1])));
	      theDevice	-> setGain (my_radioData -> theGain);
	      break;

	   case Q_SERVICE:
	      my_radioData -> serviceName = std::string ((char *)(&buffer [1]));
	      if (theRadio -> is_audioService (my_radioData -> serviceName))
	         theRadio -> dab_service (my_radioData -> serviceName);
	      break;

	   case Q_CHANNEL:
	      theRadio	-> stop ();
	      theDevice	-> stopReader ();
	      my_radioData -> theChannel = std::string ((char *)(&buffer [1]));
	      {  int frequency = theBandHandler -> Frequency (my_radioData -> theBand,
	                                              my_radioData -> theChannel);
	         theDevice	-> setVFOFrequency (frequency);
	      }
	      theRadio	-> startProcessing ();
	      theDevice	-> restartReader ();
	      break;

	   default:
	      break;
	}
}



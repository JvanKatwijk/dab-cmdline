package model;
import java.io.IOException;
import java. util. *;
import java.io.InputStream;
import java.io.DataInputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.DataOutputStream;
import java.io.OutputStreamWriter;
import javax.bluetooth.*;
import javax.microedition.io. *;

//
//	The "Model" is the actual interface to the bluetooth connector
//	The start is - obviously - building a connection, and
//	once enabled, the run function will read the incoming
//	data and pass that on to the external world.
public class ClientModel extends Thread {
//
//	keys for the outgoing messages
	private final int	Q_QUIT			= 0100;
	private	final int	Q_IF_GAIN_REDUCTION	= 0101;
	private	final int	Q_SOUND_LEVEL		= 0102;
	private	final int	Q_LNA_STATE		= 0103;
	private	final int	Q_AUTOGAIN		= 0104;
	private	final int	Q_CHANNEL		= 0105;
	private	final int	Q_SERVICE		= 0106;
	private	final int	Q_RESET			= 0107;
//
//	keys for incoming messages
	private	final int	Q_ENSEMBLE		= 0100;
	private	final int	Q_SERVICE_NAME		= 0101;
	private	final int	Q_TEXT_MESSAGE		= 0102;
	private	final int	Q_PROGRAM_DATA		= 0103;
	private	final int	Q_SIGNAL_QUALITY	= 0104;
	private	final int	Q_NEW_ENSEMBLE		= 0105;
	private	final int	Q_NO_SERVICE		= 0106;
//
//	For the connection we need
	private final StreamConnection thePlug;
	private final DataOutputStream outputter;
	OutputStreamWriter bWriter;
	private final DataInputStream inputter;
	private final InputStreamReader bReader;
//

	private	boolean autoGain	= false;
//	for signaling, we need
	private final List<modelSignals> listener = new ArrayList<>();
        public  void    addServiceListener (modelSignals service) {
           listener. add (service);
        }

	public ClientModel (StreamConnection thePlug) throws IOException {
	   this. thePlug	= thePlug;
	   outputter		= thePlug. openDataOutputStream ();
           bWriter		= new OutputStreamWriter (outputter);
           inputter		= thePlug. openDataInputStream ();
           bReader		= new InputStreamReader (inputter);
	}

	public	void startRadio () {
	   start ();
	}

	@Override 
	public void run () {
           char inBuffer[] = new char [255];
	   char header  [] = new char [3];
	   while (true) {
	      try {
	         int amount =  bReader. read (header, 0, 3);
	         amount = bReader. read (inBuffer, 0, header [2]);
//	         System. out. println ("Key " + (int) (header [0]));
                 Dispatcher (header [0], header [2], inBuffer);
	      } catch (Exception e) {
	      System. out. println ("we zijn uitgelezen");
	      }
           }
	}

//
//	This function is called from within the thread
	public void Dispatcher (char key, char segSize, char [] inBuffer) {
	   char buffer [] = new char [segSize + 1];
	   for (int i = 0; i < segSize; i ++)
	      buffer [i] = inBuffer [i];

	   switch (key) {
	      case Q_ENSEMBLE:
	         { buffer [segSize]	= 0;
	           final String ensembleLabel = String. valueOf (buffer);
	           try {
	              javax. swing. SwingUtilities.
	                         invokeLater (new Runnable () {
                          @Override
                          public void run () {
	                     show_ensembleName (ensembleLabel, 0);
	                  }});
                   } catch (Exception e) {}
	         }
	         break;

	      case Q_SERVICE_NAME:
	         {  buffer [segSize] = 0;
	            final String serviceLabel = String. valueOf (buffer);
	            try {
	               javax. swing. SwingUtilities.
	                         invokeLater (new Runnable () {
                          @Override
                          public void run () {
	                     show_serviceName (serviceLabel);
	                  }});
                    } catch (Exception e) {}
	         }
	         break;

	      case Q_TEXT_MESSAGE:
	         {  buffer [segSize] = 0;
	            final String dynamicLabel = String. valueOf (buffer);
	            try {
	                 javax. swing. SwingUtilities.
	                            invokeLater (new Runnable () {
                             @Override
                             public void run () {
	                        show_dynamicLabel (dynamicLabel);
	                     }});
                    } catch (Exception e) {}
	         }
	         break;

	      case Q_PROGRAM_DATA:
	         {  buffer [segSize] = 0;
	            final String programData = String. valueOf (buffer);
	            try {
	               javax. swing. SwingUtilities.
	                            invokeLater (new Runnable () {
                             @Override
                             public void run () {
	                        show_programData (programData);
	                     }});
                    } catch (Exception e) {}
	         }
	         break;

	      case Q_SIGNAL_QUALITY:
	         {  final int qualityInd = buffer [0];
	            try {
	               javax. swing. SwingUtilities.
	                            invokeLater (new Runnable () {
                          @Override
                          public void run () {
	                     show_signalQuality (qualityInd);
	                  }});
                    } catch (Exception e) {}
	         }
	         break;

	      case Q_NEW_ENSEMBLE:
	         try {
	            javax. swing. SwingUtilities.
	                          invokeLater (new Runnable () {
	               @Override
	               public void run () {
	                  clearScreen ();
	               }});
	         } catch (Exception e) {}
	         break;

	      case Q_NO_SERVICE:
	         try {
	              final String s = "no service found"
	              javax. swing. SwingUtilities.
	                         invokeLater (new Runnable () {
                          @Override
                          public void run () {
	                     show_dynamicLabel (s);
	                  }});
                 } catch (Exception e) {}
	         break;
	      default:;
	   }
	}

	public  void show_ensembleName (String Name, int Sid) {
           listener. forEach((hl) -> {
                hl. show_ensembleName (Name, Sid);
            });
        }

	public void show_serviceName (String s) {
           listener.forEach((hl) -> {
               hl. show_serviceName (s);
            });
        }

	public	void	show_dynamicLabel (String s) {
	   listener. forEach ((hl) -> {
	       hl. show_dynamicLabel (s);
	   });
	}

	public	void	show_programData (String s) {
	   listener. forEach ((hl) -> {
	      hl. show_programData (s);
	   });
	}

	public	void	clearScreen () {
	   listener. forEach ((hl) -> {
	      hl. clearScreen ();
	   });
	}

	public	void	show_signalQuality (int q) {
	   listener. forEach ((hl) -> {
	      hl. show_signalQuality (q);
	   });
	}

	public	void setService (String s) {
	   char data [] = new char [s. length () + 4];
	   data [0] = Q_SERVICE;
	   data [1] = 0;
	   data [2] =  (char) (s. length () + 1);
	   for (int i = 0; i < s. length (); i ++)
	      data [3 + i] = s. charAt (i);
	   data [3 + s. length ()] = 0;
	   try {
	      bWriter. write (data, 0, s. length () + 4);
	      bWriter. flush ();
	   } catch (Exception e) {}
	}

	public void	channelSelect (String s) {
	   char data [] = new char [s. length () + 4];
	   data [0] = Q_CHANNEL;
	   data [1] = 0;
	   data [2] =  (char) (s. length () + 1);
	   for (int i = 0; i < s. length (); i ++)
	      data [3 + i] = s. charAt (i);
	   data [3 + s. length ()] = 0;
	   try {
	      bWriter. write (data, 0, s. length () + 4);
	      bWriter. flush ();
	   } catch (Exception e) {}
	}

	public	void	setLnaState	(int lnaState) {
	   char data [] = new char [3 + 2];
	   data [0] = Q_LNA_STATE;
	   data [1] = 0;
	   data [2] = 2;
	   data [3] = (char)lnaState;
	   try {
	      bWriter. write (data, 0, 5);
	      bWriter. flush ();
	   } catch (Exception e) {}
	}

	public	void	gainReduction	(int GRdB) {
	   char data [] = new char [3 + 2];
	   data [0] = 0101;
	   data [1] = 0;
	   data [2] = 2;
	   data [3] = (char)GRdB;
	   try {
	      bWriter. write (data, 0, 5);
	      bWriter. flush ();
	   } catch (Exception e) {}
	}

	public	void	set_soundLevel	(int soundLevel) {
	   char data [] = new char [3 + 2];
	   data [0] = Q_SOUND_LEVEL;
	   data [1] = 0;
	   data [2] = 2;
	   data [3] = (char)soundLevel;
	   try {
	      bWriter. write (data, 0, 5);
	      bWriter. flush ();
	   } catch (Exception e) {}
	}

	public	void	set_autoGain	() {
	   char data [] = new char [3 + 2];
	   data [0] = Q_AUTOGAIN;
	   data [1] = 0;
	   data [2] = 2;
	   autoGain	= !autoGain;
	   data [3] = (char)(autoGain ? 1 : 0);
	   try {
	      bWriter. write (data, 0, 5);
	      bWriter. flush ();
	   } catch (Exception e) {}
	}

};

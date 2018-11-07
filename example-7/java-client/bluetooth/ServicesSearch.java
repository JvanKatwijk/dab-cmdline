package bluetooth;

import java.io.IOException;
import java.util.Enumeration;
import java.util.Vector;
import javax.bluetooth.*;
import javax.microedition.io. *;
/**
 *
 * Minimal Services Search example.
 */
public class ServicesSearch {

	final RemoteDeviceDiscovery mySearcher =
	                              new RemoteDeviceDiscovery ();
	final Object serviceSearchCompletedEvent = new Object ();
	static final UUID radioId	= new UUID ("0000abcd", false);
	static final UUID serviceUUID	= new UUID (0x0003);	// RFComm
	static final Vector/*<RemoteDevice>*/ devicesDiscovered =
                                                      new Vector();
	static final Vector/*<String>*/ serviceFound = new Vector();

	public StreamConnection findService ()
	                 throws IOException, InterruptedException {
//	First run RemoteDeviceDiscovery and use discovered device
	   serviceFound.clear();
	   devicesDiscovered. clear ();
	   mySearcher. findDevices (devicesDiscovered);
	   if (devicesDiscovered. size () == 0) {
	      System. out. println ("no devices found, fatal");
	      return null;
	   }
//
//	OK, we have device(s), look which one provides the "radio"
	   DiscoveryListener listener = new DiscoveryListener () {
	      public void deviceDiscovered (RemoteDevice btDevice,
	                                    DeviceClass cod) {
	      }

	      public void inquiryCompleted (int discType) {
	      }

	      public void servicesDiscovered (int transID,
	                                      ServiceRecord[] servRecord) {
	         for (int i = 0; i < servRecord.length; i++) {
	            String url = servRecord [i].
	                           getConnectionURL (ServiceRecord.NOAUTHENTICATE_NOENCRYPT, false);
	            if (url == null) {
	               continue;
	            }
	            System. out. println (url);
	            if (servRecord [i]. getAttributeValue (3) != null) {
		       if (radioId. equals (
	                        servRecord [i]. getAttributeValue (3). getValue ())) {
	                  serviceFound. add (servRecord [i]. getHostDevice (). getBluetoothAddress ());
//	                  serviceFound. add (url);
                          DataElement serviceName =
	                     servRecord [i].getAttributeValue (0x0100);
	               }
	            }
	         }
	      }

	      public void serviceSearchCompleted (int transID, int respCode) {
	         System. out. println ("service search completed!");
	         synchronized (serviceSearchCompletedEvent){
	            serviceSearchCompletedEvent. notifyAll ();
	         }
	      }
	   };

	   UUID [] searchUuidSet = new UUID[] {serviceUUID};
	   int[] attrIDs =  new int[] {
                0x0100 // Service name
	   };

	   for (Enumeration en = devicesDiscovered. elements ();
	                                       en.hasMoreElements ();) {
	      RemoteDevice btDevice = (RemoteDevice) en. nextElement ();
	      synchronized (serviceSearchCompletedEvent) {
	         LocalDevice. getLocalDevice ().
	                     getDiscoveryAgent().
	                       searchServices (attrIDs,
	                                       searchUuidSet,
	                                       btDevice, listener);
	         serviceSearchCompletedEvent. wait ();
	      }
	   }

	   if (serviceFound. size () == 0) {
	      System. out. println ("no radio found, fatal");
	      return null;
	   }
	   String url	= (String)serviceFound. firstElement ();

	   url	= "btspp://" + url + ":1;master=false;encrypt=false;authenticate=false";
	   System. out. println ("Radio Service provided by " + url);
	   StreamConnection clientSession =
	                  (StreamConnection)Connector. open (url);
	   return clientSession;
	}

	public static void Dispatcher (char [] buffer, int filled) {
	   int	starter = 0;
	   while (starter < filled) {
	      int segSize	= buffer [starter + 2];
	      int mType		= buffer [starter];
	      char message []	= new char [segSize + 1];
	      for (int i = 0; i < segSize; i ++)
	         message [i] = buffer [starter + 3 + i];
	      message [segSize] = 0;
	      String m = String. valueOf (message);
	      starter += segSize + 3;
	      switch (mType) {
	         case 0100:
	            System. out. println ("Ensemble " + m);
	            break;
	         case 0101:
	            System. out. println ("Servicename " + m);
	            break;
	         case 0102:
	            System. out. println ("Text message " + m);
	            break;
	         default:
	            System. out. println (m);
	      }
	   }
	}
}

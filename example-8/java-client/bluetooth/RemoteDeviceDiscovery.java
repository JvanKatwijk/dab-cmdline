package bluetooth;

import java.io.IOException;
import java.util.Vector;
import javax.bluetooth.*;

/**
  * Minimal Device Discovery example.
  */
public class RemoteDeviceDiscovery {
	public void findDevices (Vector devicesDiscovered)
	                      throws IOException, InterruptedException {
	   final Object inquiryCompletedEvent = new Object ();
	   devicesDiscovered. clear ();
	   DiscoveryListener listener = new DiscoveryListener () {
	      public void deviceDiscovered (RemoteDevice btDevice,
	                                    DeviceClass cod) {
	         System. out. println
	             ("Device " + btDevice.getBluetoothAddress() + " found");
	         devicesDiscovered. addElement (btDevice);
	         try {
	            System. out. println ("     name " +
	                                btDevice. getFriendlyName (false));
	         } catch (IOException cantGetDeviceName) {
	         }
	      }

	      public void inquiryCompleted (int discType) {
	         System. out. println ("Device Inquiry completed!");
	         synchronized (inquiryCompletedEvent){
	            inquiryCompletedEvent. notifyAll ();
	         }
	      }

	      public void serviceSearchCompleted (int transID, int respCode) {
	      }

	      public void servicesDiscovered (int transID,
	                                   ServiceRecord [] servRecord) {
	         System. out. println ("service found " + transID);
	      }
           };

	   synchronized (inquiryCompletedEvent) {
	      boolean started =
	             LocalDevice.getLocalDevice().getDiscoveryAgent().startInquiry(DiscoveryAgent.GIAC, listener);
	      if (started) {
	         System. out. println
	                    ("wait for device inquiry to complete...");
	         inquiryCompletedEvent. wait ();
	         System. out. println 
	                    (devicesDiscovered.size() +  " device(s) found");
	      }
	   }
	}
}

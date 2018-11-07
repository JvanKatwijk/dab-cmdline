
import	java.util.*;
import	javax. swing.*;
import	java.io.InputStream;
import	java.io.DataInputStream;
import	java.io.InputStreamReader;
import	java.io.OutputStream;
import	java.io.DataOutputStream;
import	java.io.OutputStreamWriter;
import	javax.bluetooth.*;
import	javax.microedition.io. *;

import	bluetooth.*;
import	viewer.*;
import	model.*;
import	controller.*;

public class DabClient {
//	... Create model, view, and controller.  They are
//	created once here and passed to the parts that
//	need them so there is only one copy of each.

	public static void main (String[] args) {
	   ServicesSearch serviceFinder = new ServicesSearch ();
	   final StreamConnection thePlug;
	   try {
	      thePlug	= serviceFinder. findService ();
	   } catch (Exception e) {
	      System. out. println ("could not establish connection");
	      return;
	   }
	   if (thePlug == null) {
	      System. out. println ("failed to connect, fatal\n");
	      return;
	   }
	   java.awt.EventQueue.invokeLater (new Runnable() {
              @Override
	      public void run () {
	         final ClientModel	model;
	         try {
	            model      = new ClientModel  (thePlug);
	         } catch (Exception e) {
	           System. out. println ("failing");
	           return;
	         }
                 ClientView       view       = new ClientView   ();
	         ClientController inputReader  = 
	                                  new ClientController (model, view);
	         inputReader. startRadio ();
                 view.setVisible(true);
	         view. setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
                 view. addWindowListener(new java.awt.event.WindowAdapter() {
	            @Override
	             public void
	                windowClosing(java.awt.event.WindowEvent windowEvent) {
	                if (JOptionPane.showConfirmDialog (new JFrame (),
	                    "Are you sure to close this window?",
	                                       "Really Closing?",
	                     JOptionPane.YES_NO_OPTION,
	                     JOptionPane.QUESTION_MESSAGE) ==
	                              JOptionPane.YES_OPTION){
                        }
                        System.exit(0);
                    }
	         });
	      }
	   });
	}

}


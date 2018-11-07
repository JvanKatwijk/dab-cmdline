package viewer;

import java.awt.Component;
import java.awt.Dimension;
import javax.swing.JFrame;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import  javax. swing.table.DefaultTableModel;

public class serviceTable extends JFrame {
	public JTable table = new JTable ();

	public serviceTable () {
	        table.setModel (new javax.swing.table.DefaultTableModel(
                                new Object [][] {
                                {null}
                                },
                                new String [] {
                                "service name"
                                }
                               ));

//	add the table to the frame
            Component add;
            add = this. add (new JScrollPane(table));
         
	   this. setTitle ("DAB services");
//	   this. setDefaultCloseOperation (JFrame.EXIT_ON_CLOSE);       
	   this. setPreferredSize (new Dimension (100, 200));
	   this. pack();
	   this. setVisible (true);
	}

	public	void	clearTable () {
	   DefaultTableModel dm = (DefaultTableModel)table. getModel ();
	   while (dm. getRowCount () > 0)
	      dm. removeRow (0);
	}

	public void newService (String name) {
	   ((DefaultTableModel)table. getModel ()).
	       addRow (new Object [] {name});
	}

	public String get_serviceName (int row, int column) {
	   Object o	= table. getValueAt (row, column);
	   if (!(o instanceof String))
	      return "?????";
	   return (String)o;
	}
}


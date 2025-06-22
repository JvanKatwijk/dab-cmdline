
#include	"tii-handler.h"

	tiiHandler::tiiHandler	() {
	has_dataBase	= false;
	std::string home_dir = getenv ("HOME");
	std::string fileName = home_dir + "/.txdata.tii";
	fprintf (stderr, "de filename = %s\n", fileName. c_str ());
	the_dataBase = theReader. readFile (fileName);
	fprintf (stderr, "size %d\n", the_dataBase. size ());
	if (the_dataBase. size () > 10)
	   has_dataBase = true;
	threadHandle	= std::thread (&tiiHandler::run, this);
}

	tiiHandler::~tiiHandler	() {
	if (running. load ()) {
	   running. store (false);
	   threadHandle. join ();
	}
}

void	tiiHandler::add (tiiData p) {
	if (running. load ())
	   theBuffer. push (p);
}

void	tiiHandler::run	() {
	running. store (true);
	while (running. load ()) {
	   while (theBuffer. empty ())
	      usleep (50000);
	   while (!theBuffer. empty ()) {
	      tiiData xx = theBuffer. pop ();
	      if (known (xx))
	         continue;
	      if ((xx. ecc == 0) || (xx. EId == 0)) {
	         fprintf (stderr, "No further data: %d %d \n",
	                                                 xx. mainId,
	                                                 xx. subId);
	         tiiTable. push_back  (xx);
	         continue;
	      }
	      if (!has_dataBase) {
	         fprintf (stderr, "No database available: %d %X %d %d\n",
	                              xx. ecc, xx. EId, xx. mainId, xx. subId);
	         tiiTable. push_back (xx);
	         continue;
	      }

	      cacheElement *handle = lookup (xx);	// in tii database
	      if (handle == nullptr) 
	         fprintf (stderr, "not found %X %d %d\n",
	                            xx. EId, xx. mainId, xx. subId);	
	      else 
	         fprintf (stderr, "%d %d -> %s %s %s %s %f %f\n",
	                        handle -> mainId,
	                        handle -> subId,
	                        handle -> country. c_str (),
	                        handle -> channel. c_str (),
	                        handle -> ensemble. c_str (),
	                        handle -> transmitterName. c_str (),
	                        (double)handle -> latitude,
	                        (double)handle -> longitude);
	      tiiTable. push_back (xx);
	   }
	}
}

bool	tiiHandler::known (tiiData &t) {
	if (!running. load ())
	   return true;
	for (auto &tableElement: tiiTable) {
	   if ((t. mainId == tableElement. mainId) &&
	       (t. subId == tableElement. subId))
	      return true;
	}
	return false;
}

cacheElement *tiiHandler::lookup (tiiData &tii) {
	for (auto &ce : the_dataBase) {
	   if (ce. Eid != tii. EId)
	      continue;
	   if ((ce. mainId == tii. mainId) && (ce. subId == tii. subId)) {
	      return &ce;
	   }
	}
	return nullptr;
}
	       

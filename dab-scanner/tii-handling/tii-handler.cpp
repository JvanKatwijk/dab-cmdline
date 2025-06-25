
#include	"tii-handler.h"

	tiiHandler::tiiHandler	() {
	std::string home_dir = getenv ("HOME");
	std::string fileName = home_dir + "/.txdata.tii";
	the_dataBase = theReader. readFile (fileName);
	if (the_dataBase. size () > 10)
	   has_dataBase = true;
	else
	   fprintf (stderr, "Note: no database found,\n output is limited\n");
}

	tiiHandler::~tiiHandler	() {
	if (running. load ()) {
	   running. store (false);
	   threadHandle. join ();
	}
}

void	tiiHandler::stop	() {
	if (running. load ()) {
	   running. store (false);
	   threadHandle. join ();
	}
}

void	tiiHandler::start	(uint32_t Eid) {
	tiiTable. resize (0);
	while (!theBuffer. empty ())
	   (void)theBuffer. pop ();
	currentEid	= Eid;
	threadHandle	= std::thread (&tiiHandler::run, this);
}

void	tiiHandler::add (tiiData p) {
	if (running. load ())
	   theBuffer. push (p);
}

void	tiiHandler::run	() {
	running. store (true);
	while (running. load ()) {
	   while (theBuffer. empty () && running. load ())
	      usleep (50000);
	   while (!theBuffer. empty ()) {
	      tiiData xx = theBuffer. pop ();
	      xx. EId	= currentEid;
	      if (known (xx))
	         continue;
	      if ((xx. ecc == 0) || (xx. EId == 0)) {
	         fprintf (stderr, "No further data available to decode: %d %d \n",
	                                                 xx. mainId,
	                                                 xx. subId);
//	         tiiTable. push_back  (xx);
	         continue;
	      }
	      if (!has_dataBase) {
	         fprintf (stderr, "No database available: %d %X %d %d\n",
	                              xx. ecc, xx. EId, xx. mainId, xx. subId);
//	         tiiTable. push_back (xx);
	         continue;
	      }

	      cacheElement *handle = lookup (xx);	// in tii database
	      if (handle == nullptr) { 
	         fprintf (stderr, "not found in database %X %d %d\n",
	                            xx. EId, xx. mainId, xx. subId);	
	         continue;
	      }
	      else {
	         handle	-> strength = xx. strength;
	         tiiTable. push_back (*handle);
	      }
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

void	tiiHandler::print	() {
	for (auto &ce : tiiTable) 
	   fprintf (stderr, " %X %d %d\t-> %f %s %s %s %s %f %f (%2f %d %d)\n",
	                        ce. Eid,
	                        ce. mainId,
	                        ce. subId,
	                        ce. strength,
	                        ce. country. c_str (),
	                        ce. channel. c_str (),
	                        ce. ensemble. c_str (),
	                        ce. transmitterName. c_str (),
	                        (double)ce. latitude,
	                        (double)ce. longitude,
	                        (float)ce. power,
	                        (int)ce. altitude,
	                        (int)ce. height);
}


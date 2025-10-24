
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
}

void	tiiHandler::add (tiiData p) {
	if (!has_dataBase)
	   return;
	p. EId	= currentEid;
	if (known (p))
	   return;
	if ((p. ecc == 0) || (p. EId == 0)) 
	   return;

	cacheElement *handle = lookup (p);	// in tii database
	if (handle == nullptr) { 
	   return;
	}
	else {
	   handle	-> strength = p. strength;
	   tiiTable. push_back (*handle);
	}
}

bool	tiiHandler::known (tiiData &t) {
	if (!has_dataBase)
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

int	tiiHandler::nrTransmitters	() {
	return tiiTable. size ();
}

cacheElement	tiiHandler::deliver (int index) {
cacheElement dummy;
	if ((int)(tiiTable. size ()) < index)
	   return dummy;
	return tiiTable. at (index);
}



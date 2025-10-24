

	jsonPrinter::jsonPrinter	(FILE *f) {
	this	-> file	= f;
	}

	jsonPrinter::~jsonPrinter	() {
	}

	
void	jsonPrinter::print_fileHeader	() {}
void	jsonPrinter::print_ensembleData (void *theRadio,
	                                 std::string channel,
	                                 std::string ensembleLabel,
	                                 uint32_t ensembleId,
	                                 bool *firstEnsemble);
void	jsonPrinter::print_audioheader	();
void	jsonPrinter::print_audioService (void *theRadio,
	                                 std::string serviceName,
	                                 std::string currentChannel,
	                                 audiodata *d,
	                                 bool *firstService);
void	jasonPrinter::print_dataHeader	();
void	print_dataService		(void	*theRadio,
	                                 std::string	serviceName,
	                                 std::string  currentChannel,
	                                 uint8_t	compnr,
	                                 packetdata *d,
					 bool *firstService);
void	jsonPrinter::print_ensembleFooter ();
void	jsonPrinter::print_fileFooter	();

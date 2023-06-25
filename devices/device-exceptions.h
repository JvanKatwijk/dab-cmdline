#include <stdexcept>

#define MAX_MESSAGE_SIZE 256

// Exceptions with input
class LibNotFound : public std::exception {
    private:
      char message[MAX_MESSAGE_SIZE];

    public:
	LibNotFound (const char * libraryString) {
	   snprintf (this -> message, MAX_MESSAGE_SIZE, "Library %s not found", libraryString);
      };

      const char * what () const noexcept override  {
          return message;
      }

};

class LoadingSymbolsFailed : public std::exception {
    private:
      char message[MAX_MESSAGE_SIZE];

    public:
      LoadingSymbolsFailed(const char * libraryString) {
        snprintf(this->message, MAX_MESSAGE_SIZE, "Unable to load symbols/functions from %s", libraryString);
      };

      const char * what () const noexcept override  {
          return message;
      }

};

class CreatingSocketFailed : public std::exception {
    private:
      char message[MAX_MESSAGE_SIZE];

    public:
      CreatingSocketFailed(char * error) {
        snprintf(this->message, MAX_MESSAGE_SIZE, "Unable to open socket because %s", error);
      };

      const char * what () const noexcept override  {
          return message;
      }

};

class GettingHostnameFailed : public std::exception {
    private:
      char message[MAX_MESSAGE_SIZE];

    public:
      GettingHostnameFailed(char * error) {
        snprintf(this->message, MAX_MESSAGE_SIZE, "Unable to get hostname because: %s", error);
      };

      const char * what () const noexcept override  {
          return message;
      }

};

class ConnectingFailed : public std::exception {
    private:
      char message[MAX_MESSAGE_SIZE];

    public:
      ConnectingFailed(char * error) {
        snprintf(this->message, MAX_MESSAGE_SIZE, "Unable to connect because: %s", error);
      };

      const char * what () const noexcept override  {
          return message;
      }

};

class OpeningFileFailed : public std::exception {
    private:
      char message[MAX_MESSAGE_SIZE];

    public:
      OpeningFileFailed(char * file ,char * error) {
        snprintf(this->message, MAX_MESSAGE_SIZE, "Unable to open %s because: %s", file, error);
      };

      const char * what () const noexcept override  {
          return message;
      }

};

class OpeningChannelFailed : public std::exception {
    private:
      char message[MAX_MESSAGE_SIZE];

    public:
	OpeningChannelFailed(char * direction ,char * channel) {
	   snprintf (this->message, MAX_MESSAGE_SIZE,
	       "Unable to open/enable %s channel %s", direction, channel);
      };

      const char * what () const noexcept override  {
          return message;
      }

};

// All exceptions below take no arguments

class DeviceNotFound : public std::exception {
    private:
    public:
      DeviceNotFound(){};

      const char * what () const noexcept override {
          return "Device not found";
      }
};

class OpeningFailed : public std::exception {
    private:
    public:
      OpeningFailed(){};

      const char * what () const noexcept override {
          return "Unable to open device";
      }
};

class SampleRateFailed : public std::exception {
    private:
    public:
      SampleRateFailed(){};

      const char * what () const noexcept override {
          return "Unable to set sample rate";
      }
};

class WrongSdrPlayVersion : public std::exception {
    private:
    public:
      WrongSdrPlayVersion(){};

      const char * what () const noexcept override {
          return "Please upgrade to sdrplay version 2.13";
      }
};

class StartingThreadFailed : public std::exception {
    private:
    public:
      StartingThreadFailed(){};

      const char * what () const noexcept override {
          return "Unable to start thread";
      }
};

class NoInstanceFound : public std::exception {
    private:
    public:
      NoInstanceFound(){};

      const char * what () const noexcept override {
          return "No instance found";
      }
};

class BandwidthFailed : public std::exception {
    private:
    public:
	BandwidthFailed(){};

	const char *what () const noexcept override {
	   return "Unable to set bandwith";
	}
};

class PortSelectFailed : public std::exception {
private:
    public:
	PortSelectFailed(){};
	const char * what () const noexcept override {
	   return "Unable to set port";
	}
};

class InterfaceFailed : public std::exception {
    private:
    public:
      InterfaceFailed(){};

      const char * what () const noexcept override {
          return "Unable to find interface";
      }
};

class FrequencyFailed : public std::exception {
    private:
    public:
      FrequencyFailed(){};

      const char * what () const noexcept override {
          return "Unable to set frequency";
      }
};

class CreatingBufferFailed : public std::exception {
    private:
    public:
      CreatingBufferFailed(){};

      const char * what () const noexcept override {
          return "Unable to create buffer";
      }
};

class InitFailed : public std::exception {
    private:
    public:
      InitFailed(){};

      const char * what () const noexcept override {
          return "Unable to initalize device";
      }
};

class GetChannelsFailed : public std::exception {
    private:
    public:
      GetChannelsFailed(){};

      const char * what () const noexcept override {
          return "Unable to get channels";
      }
};

class UnknownError : public std::exception {
    private:
    public:
      UnknownError(){};

      const char * what () const noexcept override {
          return "Unknown idk either";
      }
};

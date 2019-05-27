
all:	hackrf-server

hackrf-server	: main.cpp ringbuffer.h hackrf-server.o iq-input.o hackrf-handler.o lowpass.o
	g++ -g -o hackrf-server main.cpp hackrf-server.o iq-input.o hackrf-handler.o lowpass.o  -lpthread -lsamplerate -lm -lstdc++ -ldl

iq-input.o	: iq-input.cpp iq-input.h ringbuffer.h
	g++ -I . -c iq-input.cpp

hackrf-handler.o	: hackrf-handler.cpp hackrf-handler.h ringbuffer.h hackrf.h
	g++ -I . -c hackrf-handler.cpp

hackrf-server.o	: hackrf-server.cpp hackrf-server.h hackrf-handler.h ringbuffer.h lowpass.h
	g++ -I . -c hackrf-server.cpp

lowpass.o	: lowpass.cpp lowpass.h
	g++ -I . -c lowpass.cpp

clean	:
	rm *.o hackrf-server


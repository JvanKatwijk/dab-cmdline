#
TEMPLATE    = app
CONFIG      += console
QT          += core gui widgets

INCLUDEPATH += . \ 
	       ..

HEADERS     = ./client.h \
	      ../protocol.h \
	      ./bluetooth-handler.h \
	      ./bluetooth.h \
	      ../band-handler.h
	

SOURCES		=  ./client.cpp  \
	           main.cpp \
	           bluetooth-handler.cpp \	
	           ../band-handler.cpp

TARGET		= Client
FORMS		+= ./widget.ui
LIBS		+= -L. -lwrapper
unix{
DESTDIR     = .
}


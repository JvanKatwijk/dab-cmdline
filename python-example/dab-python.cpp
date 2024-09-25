#
/*
 *    Copyright (C) 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is the python wrapper for the DAB-library.
 *    DAB-library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB-library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB-library, if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include	<Python.h>
#include	<numpy/arrayobject.h>
#include	"dab-api.h"
#include	"includes/support/band-handler.h"
#ifdef	HAVE_SDRPLAY
#include	"sdrplay-handler.h"
#elif HAVE_AIRSPY
#include	"airspy-handler.h"
#elif HAVE_RTLSDR
#include	"rtlsdr-handler.h"
#else
#include	"device-handler.h"
#endif
//
//	wrapper for the python interface to the dab-library
//	ensure that the function names are not mangled
//
extern "C" {
PyObject *dabInit_p		(PyObject *self, PyObject *args);
PyObject *dabExit_p		(PyObject *self, PyObject *args);
PyObject *dabReset_p		(PyObject *self, PyObject *args);
PyObject *dabStop_p		(PyObject *self, PyObject *args);
PyObject *dabService_p		(PyObject *self, PyObject *args);
PyObject *dabgetSId_p		(PyObject *self, PyObject *args);
PyObject *PyInit_libdab_lib	(void);
}
//
//	deviceHandler is local to the python interface
deviceHandler *theDevice;
API_struct *interface;
//
//	We need to define 'C' callback functions, whose purpose
//	is to call the appropriate Python function
//	We keep it simple and implement only a few callback functions,
//	the ones we think minimally needed
//	
//	typedef	void (*syncsignal_t)	(bool,		// time synced or not
//	                                 void *);
//	typedef void (*ensemblename_t)	(char *,	// name
//	                                 int32_t,	// Hex code
//	                                 void *);
//	typedef	void (*programname_t)	(char *,	// name
//	                                 int32_t,	// Id
//	                                 void *);
//	typedef void (*audioOut_t)	(int16_t *,	// buffer
//	                                 int32_t,	// size
//	                                 int32_t,	// samplerate
//	                                 bool,		// stereo
//	                                 void *);	// context
//	typedef	void (*dataOut_t)	(char *,
//	                                 void *);
//	typedef void (*bytesOut_t)	(uint8_t *, 
//	                                 int16_t,
//	                                 void *);
//	typedef	void (*systemdata_t)	(bool,
//	                                 int16_t,
//	                                 int32_t,
//	                                 void *);
//	typedef void (*fib_quality_t)	(int16_t,
//	                                 void *);
//	typedef	void (*programQuality_t)(int16_t,
//	                                 int16_t,
//	                                 int16_t,
//	                                 void *);
//
//	Not (yet?) implemented
//	typedef void (*programdata_t)	(audiodata_t *,
//	                                 void	*);


//	typedef	void (*syncsignal_t)	(bool, void *);
PyObject *callbackSyncSignal = NULL;
static
void	callback_syncSignal (bool b, void *ctx) {
PyObject	*arglist;
PyObject	*result;
PyGILState_STATE gstate;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("(b)", b);
	result  = PyObject_CallObject (callbackSyncSignal, arglist);
	if (arglist != NULL)
	   Py_DECREF (arglist);
	if (result != NULL)
	   Py_DECREF (result);
	PyGILState_Release (gstate);
}

//	typedef	void (*systemdata_t)	(bool,	
//	                                 int16_t,
//	                                 int32_t,
//	                                 void *);
PyObject *callbackSystemData	= NULL;
static
void	callback_systemData (bool b, int16_t snr, int32_t offs, void *ctx) {
PyObject *arglist;
PyObject *result;
PyGILState_STATE gstate;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("(bhi)", b, snr, offs);
	result  = PyObject_CallObject (callbackSystemData, arglist);
	if (arglist != NULL)
	   Py_DECREF (arglist);
	if (result != NULL)
	   Py_DECREF (result);
	PyGILState_Release (gstate);
}

//	typedef void (*fib_quality_t)	(int16_t,
//	                                 void *);
PyObject *callbackFibQuality	= NULL;
static
void	callback_fibQuality (int16_t q, void *ctx) {
PyObject *arglist;
PyObject *result;
PyGILState_STATE gstate;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("(h)", q);
	result  = PyObject_CallObject (callbackFibQuality, arglist);
	if (arglist != NULL)
	   Py_DECREF (arglist);
	if (result)
	   Py_DECREF (result);
	PyGILState_Release (gstate);
}
//	typedef	void (*programQuality_t)(int16_t,
//	                                 int16_t,
//	                                 int16_t,
//	                                 void *);
//
PyObject *callbackProgramQuality	= NULL;
static
void	callback_programQuality (int16_t c1,
	                         int16_t c2, int16_t c3, void *ctx) {
PyObject *arglist;
PyObject *result;
PyGILState_STATE gstate;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("hhh", c1, c2, c3);
	result  = PyObject_CallObject (callbackProgramQuality, arglist);
	if (arglist != NULL)
	   Py_DECREF (arglist);
	if (result != NULL)
	   Py_DECREF (result);
	PyGILState_Release (gstate);
}

static
PyObject *callbackEnsembleName	= NULL;
static
void	callback_ensembleName (const char *s, int32_t id, void *ctx) {
PyGILState_STATE gstate;
PyObject *arglist;
PyObject *result;

	fprintf (stderr, "ensemblename %s. id %d\n", s, id);
	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("(si)", s, id);
	result  = PyObject_CallObject (callbackEnsembleName, arglist);
	if (arglist != NULL)
	   Py_DECREF (arglist);
	if (result != NULL)
	   Py_DECREF (result);
	PyGILState_Release (gstate);
}

static
PyObject *callbackProgramName	= NULL;
static
void	callback_programName (const char * s, int32_t id, void *ctx) {
PyGILState_STATE gstate;
PyObject *arglist;
PyObject *result;

	fprintf (stderr, "programname %s (%d)\n", s, id);
	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("(si)", s, id);
	result  = PyObject_CallObject (callbackProgramName, arglist);
	if (arglist != NULL)
	   Py_DECREF (arglist);
	if (result != NULL)
	   Py_DECREF (result);
	PyGILState_Release (gstate);
}

//	typedef void (*audioOut_t)(int16_t *, int32_t, int32_t, void *);
PyObject *callbackAudioOut	= NULL;
static
void	callback_audioOut (int16_t *b, int32_t size, int32_t rate, void *ctx) {
PyObject *arglist;
PyObject *result;
PyObject *theArray;
PyGILState_STATE gstate;
npy_intp dims [2];
float data [size / 2][2];
int	i;

	dims [0] = size / 2;
	dims [1] =  2;
	if (size == 0)
	   return;
	for (i = 0; i < size / 2; i ++) {
	   data [i] [0] = b [2 * i] / 32767.0;
	   data [i] [1] = b [2 * i + 1] / 32767.0;
	}
	gstate	= PyGILState_Ensure ();
	theArray = PyArray_SimpleNewFromData (2, dims,
	                            NPY_FLOAT, (void *)data);
	arglist = Py_BuildValue ("Oii", theArray, size / 2, rate);
	result  = PyObject_CallObject (callbackAudioOut, arglist);
	if (arglist != NULL)
	   Py_DECREF (arglist);
	if (theArray != NULL)
	   Py_DECREF (theArray);
	if (result != NULL)
	   Py_DECREF (result);
	PyGILState_Release (gstate);
}

PyObject *callbackMotData	= NULL;
static
void  callback_motData (uint8_t * data, int size,
                         const char *name, int d, void *ctx) {
  PyObject *arglist;
  PyObject *result;
  PyObject *theArray;
  PyGILState_STATE gstate;

  gstate	= PyGILState_Ensure ();
  theArray = PyBytes_FromStringAndSize((const char*)data, size);
	arglist = Py_BuildValue ("(Ozi)", theArray, name, d);
	result  = PyObject_CallObject (callbackMotData, arglist);
	if (arglist != NULL)
     Py_DECREF (arglist);
	if (theArray != NULL)
	   Py_DECREF (theArray);
	if (result != NULL)
	   Py_DECREF (result);
	PyGILState_Release (gstate);
	(void)result;
  (void)ctx;
}


//	typedef	void (*dataOut_t)	(char*, void *ctx);
PyObject *callbackDataOut	= NULL;
static
void	callback_dataOut (const char * str, void *ctx) {
PyObject *arglist;
PyObject *result;
PyGILState_STATE gstate;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("(z)", str);
	result  = PyObject_CallObject (callbackDataOut, arglist);
	if (arglist != NULL)
	   Py_DECREF (arglist);
	if (result != NULL)
	   Py_DECREF (result);
	PyGILState_Release (gstate);
	(void)result;
}

//	typedef	void (*programdata_t)	(audiodata *, void *ctx);
PyObject *callbackProgramData	= NULL;
static
void	callback_programdata (audiodata *d, void *ctx) {
PyObject *arglist;
PyObject *result;
PyGILState_STATE gstate;

	if (!d -> defined) 
	   return;
	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("hhhhh", d -> subchId,
	                                  d -> startAddr,
	                                  d -> length,
	                                  d -> bitRate,
	                                  d -> ASCTy);
	result  = PyObject_CallObject (callbackProgramData, arglist);
	if (arglist != NULL)
	   Py_DECREF (arglist);
	if (result != NULL)
	   Py_DECREF (result);
	PyGILState_Release (gstate);
	(void)result;
}

/////////////////////////////////////////////////////////////////////////
//	Here the real API starts
//
//	We do not support spectra or so.
//
//	void	*dabInit  (deviceHandler	*,
//	                   uint8_t,	// dab Mode
//	                   RingBuffer<std::complex<float>> *spectrumBuffer,
//	                   RingBuffer<std::complex<float>> *iqBuffer,
//	                   syncsignal_t        syncsignalHandler,
//	                   systemdata_t        systemdataHandler,
//	                   ensemblename_t      ensemblenameHandler,
//	                   programname_t       programnamehandler,
//	                   fib_quality_t       fib_qualityHandler,
//	                   audioOut_t          audioOut_Handler,
//	                   dataOut_t           dataOut_Handler,
//	                   bytesOut_t	       bytesOut_Handler,
//	                   programdata_t       programdataHandler,
//	                   programQuality_t    program_qualityHandler,
//			   void		       *s,
//	                   void                *userData);


int32_t	frequency;

PyObject *dabInit_p (PyObject *self, PyObject *args) {
int	theMode	= 127;
char	*theChannel;
int16_t	theGain;
PyObject	*cbsH;		// callback for syncsignalHandler
PyObject	*cbsD;		// callback for systemdataHandler
PyObject	*cbeN;		// callback for ensemble name
PyObject	*cbpN;		// callback for program name
PyObject	*cbfQ;		// callback for fib quality
PyObject	*cbaO;		// callback for audioOut
PyObject	*cbdO;		// callback for dataOut
PyObject	*cbmO;		// callback for motData
PyObject	*cbpD;		// callback for program data
PyObject	*cbpQ;		// callback for program Quality
bandHandler	dabBand;
void	*result;
	interface = new API_struct;

	(void)PyArg_ParseTuple (args,
	                        "shhOOOOOOOOOO",
	                         &theChannel,
	                         &theGain,
	                         &theMode,
	                         &cbsH,
	                         &cbsD,
	                         &cbeN,
	                         &cbpN,
	                         &cbfQ,
	                         &cbaO,
	                         &cbdO,
	                         &cbmO,
	                         &cbpD,
	                         &cbpQ
	                       );
//
//	The callbacks should be callable

	if (!PyCallable_Check (cbsH)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for audio must be callable");
	   goto err;
	}

	if (!PyCallable_Check (cbsD)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for data must be callable");
	   goto err;
	}

	if (!PyCallable_Check (cbeN)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for ensemble must be callable");
	   goto err;
	}

	if (!PyCallable_Check (cbpN)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for program data must be callable");
	   goto err;
	}

	if (!PyCallable_Check (cbfQ)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for ensemble must be callable");
	   goto err;
	}

	if (!PyCallable_Check (cbaO)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for program data must be callable");
	   goto err;
	}

	if (!PyCallable_Check (cbdO)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for ensemble must be callable");
	   goto err;
	}

	if (!PyCallable_Check (cbmO)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for ensemble must be callable");
	   goto err;
	}

	if (!PyCallable_Check (cbpD)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for ensemble must be callable");
	   goto err;
	}

	if (!PyCallable_Check (cbpQ)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for program data must be callable");
	   goto err;
	}

	Py_XINCREF (cbsH);
	if (callbackSyncSignal != NULL)
	   Py_XDECREF (callbackSyncSignal);
	callbackSyncSignal = cbsH;

	Py_XINCREF (cbsD);
	if (callbackSystemData != NULL)
	   Py_XDECREF (callbackSystemData);
	callbackSystemData	= cbsD;

	Py_XINCREF (cbeN);
	if (callbackEnsembleName != NULL)
	   Py_XDECREF (callbackEnsembleName);
	callbackEnsembleName	= cbeN;

	Py_XINCREF (cbpN);
	if (callbackProgramName != NULL)
	   Py_XDECREF (callbackProgramName);
	callbackProgramName	= cbpN;

	Py_XINCREF (cbfQ);
	if (callbackFibQuality != NULL)
	   Py_XDECREF (callbackFibQuality);
	callbackFibQuality	= cbfQ;

	Py_XINCREF (cbaO);
	if (callbackAudioOut != NULL)
	   Py_XDECREF (callbackAudioOut);
	callbackAudioOut	= cbaO;

	Py_XINCREF (cbdO);
	if (callbackDataOut != NULL)
	   Py_XDECREF (callbackDataOut);
	callbackDataOut	= cbdO;

	Py_XINCREF (cbmO);
	if (callbackMotData != NULL)
	   Py_XDECREF (callbackMotData);
	callbackMotData	= cbmO;

	Py_XINCREF (cbpD);
	if (callbackProgramData != NULL)
	   Py_XDECREF (callbackProgramData);
	callbackProgramData = cbpD;

	Py_XINCREF (cbpQ);
	if (callbackProgramQuality != NULL)
	   Py_XDECREF (callbackProgramQuality);
	callbackProgramQuality	= cbpQ;

	frequency	= dabBand. Frequency (BAND_III, theChannel);
	try {
#ifdef	HAVE_SDRPLAY
	   theDevice	= new sdrplayHandler (frequency,
	                                      0,
	                                      theGain,
	                                      4,
	                                      true,
	                                      0,
	                                      0);
#elif	HAVE_AIRSPY
	   theDevice	= new airspyHandler (frequency,
	                                     0,
	                                     theGain);
#elif	HAVE_RTLSDR
	   theDevice	= new rtlsdrHandler (frequency,
	                                     0,		// ppm offset
	                                     theGain,	// the gain
	                                     false);	// autogain
#else
	   theDevice	= new deviceHandler ();
#endif
	fprintf (stderr, "device installed\n");
	}
	catch (int e) {
	   fprintf (stdout, "allocating device failed\n");
	   exit (21);
	}

	interface -> dabMode	= theMode;
	interface -> syncsignal_Handler	= (syncsignal_t)callback_syncSignal;
	interface -> systemdata_Handler	= (systemdata_t)callback_systemData;
	interface -> ensemblename_Handler	= callback_ensembleName;
	interface -> programname_Handler	= callback_programName;
	interface -> fib_quality_Handler	= (fib_quality_t)callback_fibQuality;
	interface -> audioOut_Handler	= (audioOut_t)callback_audioOut;
	interface -> dataOut_Handler	= (dataOut_t)callback_dataOut;
	interface -> bytesOut_Handler	= nullptr;
	interface -> programdata_Handler	= (programdata_t)callback_programdata;
	interface -> program_quality_Handler =
	                                  (programQuality_t)callback_programQuality;
	interface -> motdata_Handler	= callback_motData;
	interface -> tii_data_Handler	= nullptr;
	interface -> timeHandler		= nullptr;
	result = dabInit (theDevice,
	                  interface,
	                  nullptr,		// no spectrum shown
	                  nullptr,	        // no constellation
	                  nullptr	        //
	                 );
	if (result == NULL) {
	   fprintf (stdout, "Sorry, did not work\n");
	   goto err;
	}

	theDevice       -> setGain (theGain);
//	if (autogain)
	   theDevice    -> set_autogain (true);
        theDevice       -> restartReader (frequency);
//

	return PyCapsule_New (result, "library_object", NULL);

err:
	Py_RETURN_NONE;
}

PyObject *dabExit_p (PyObject *self, PyObject *args) {
PyObject *handle_capsule;
void	 *handle;

	PyArg_ParseTuple (args, "O", &handle_capsule);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	dabExit (handle);
	Py_INCREF (Py_None);
	Py_RETURN_NONE;
}

PyObject *startProcessing_p (PyObject *self, PyObject *args) {
PyObject	*handle_capsule;
void		*handle;
PyObject	*result;
bool	b;

	PyArg_ParseTuple (args, "O", &handle_capsule);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	b = theDevice	-> restartReader (frequency);
	if (b) { 
	   fprintf (stdout, "device restarted\n");
	   dabStartProcessing (handle);
	}
	else
	   fprintf (stdout, "restart failed\n");
	result = Py_BuildValue ("(b)", b);
	Py_INCREF (result);
	return result;
}

PyObject *dabReset_p (PyObject *self, PyObject *args) {
PyObject	*handle_capsule;
void		*handle;

	PyArg_ParseTuple (args, "O", &handle_capsule);

	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	dabReset (handle);
	Py_INCREF (Py_None);
	Py_RETURN_NONE;
}

PyObject *dabStop_p (PyObject *self, PyObject *args) {
PyObject	*handle_capsule;
void		*handle;

	PyArg_ParseTuple (args, "O", &handle_capsule);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	dabStop (handle);
	theDevice	-> stopReader ();
	Py_INCREF (Py_None);
	Py_RETURN_NONE;
}

//	bool	dab_Service	(std::string, void *handle);
PyObject *dabService_p (PyObject *self, PyObject *args) {
PyObject	*handle_capsule;
void		*handle;
char		*s;

	PyArg_ParseTuple (args, "sO", &s, &handle_capsule);
	fprintf (stdout, "%s zou moeten worden gestart\n", s);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	audiodata ad;
	dataforAudioService (handle, s, &ad, 0);
	if (ad. defined)
	  set_audioChannel (handle, &ad);
	Py_RETURN_NONE;
}

static PyMethodDef module_methods [] = {
	{"dabInit",	dabInit_p,	METH_VARARGS, ""},
	{"dabExit",	dabExit_p,	METH_VARARGS, ""},
	{"dabReset",	dabReset_p,	METH_VARARGS, ""},
	{"dabProcessing", startProcessing_p, METH_VARARGS, ""},
	{"dabStop",	dabStop_p,	METH_VARARGS, ""},
	{"dabService",	dabService_p,	METH_VARARGS, ""},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef dab_lib = {
	PyModuleDef_HEAD_INIT,
	"dab_lib",
	"",
	-1,
	module_methods,
	NULL,
	NULL,
	NULL,
	NULL
};

PyObject *PyInit_libdab_lib (void) {
	import_array ();
	return PyModule_Create (&dab_lib);
}


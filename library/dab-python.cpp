#
/*
 *    Copyright (C) 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
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
//	We need to define 'C' callback functions, whose purpose
//	is to call the appropriate Python function
//	We keep it simple abd implement only a few callback functions,
//	the ones we think minimally needed
//	
//	typedef	void (*syncsignal_t)	(bool,		// time synced or not
//	                                 void *);
//	typedef void (*ensemblename_t)	(std::string,	// name
//	                                 int32_t,	// Hex code
//	                                 void *);
//	typedef	void (*programname_t)	(std::string,	// name
//	                                 int32_t,	// Id
//	                                 void *);
//	typedef void (*audioOut_t)	(int16_t *,	// buffer
//	                                 int32_t,	// size
//	                                 int32_t,	// samplerate
//	                                 bool,		// stereo
//	                                 void *);	// context
//	typedef	void (*dataOut_t)	(std::string,
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
	arglist = Py_BuildValue ("bO", b, NULL);
	result  = PyEval_CallObject (callbackSyncSignal, arglist);
	Py_DECREF (arglist);
	PyGILState_Release (gstate);
	(void)result;
}

static
PyObject *callbackEnsembleName	= NULL;
static
void	callback_ensembleName (std::string s, int32_t id, void *ctx) {
PyGILState_STATE gstate;
PyObject *arglist;
PyObject *result;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("(siO)", s, id, ctx);
	result  = PyEval_CallObject (callbackEnsembleName, arglist);
	Py_DECREF (arglist);
	PyGILState_Release (gstate);
	(void)result;
}

static
PyObject *callbackProgramName	= NULL;
static
void	callback_programName (std::string s, int32_t id, void *ctx) {
PyGILState_STATE gstate;
PyObject *arglist;
PyObject *result;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("(siO)", s. c_str (), id, NULL);
	result  = PyEval_CallObject (callbackProgramName, arglist);
	Py_DECREF (arglist);
	PyGILState_Release (gstate);
	(void)result;
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
	arglist = Py_BuildValue ("OiiO", theArray, size / 2, rate, NULL);
	result  = PyEval_CallObject (callbackAudioOut, arglist);
	Py_DECREF (arglist);
	Py_DECREF (theArray);
	PyGILState_Release (gstate);
	(void)result;
}

//	typedef	void (*dataOut_t)	(std::string, void *ctx);
PyObject *callbackDataOut	= NULL;
static
void	callback_dataOut (std::string str, void *ctx) {
PyObject *arglist;
PyObject *result;
PyGILState_STATE gstate;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("z", str. c_str ());
	result  = PyEval_CallObject (callbackDataOut, arglist);
//	Py_DECREF (arglist);
	PyGILState_Release (gstate);
	(void)result;
}

//	typedef	void (*systemdata_t)	(bool,	
//	                                 int16_t,
//	                                 int32_t,
//	                                 void *);
PyObject *callbackSystemData	= NULL;
static
void	callback_systemdata (bool b, int16_t snr, int32_t offs, void *ctx) {
PyObject *arglist;
PyObject *result;
PyGILState_STATE gstate;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("bhiO", b, snr, offs, ctx);
	result  = PyEval_CallObject (callbackSystemData, arglist);
//	Py_DECREF (arglist);
	PyGILState_Release (gstate);
	(void)result;
}

//	typedef void (*fib_quality_t)	(int16_t,
//	                                 void *);
PyObject *callbackFibQuality	= NULL;
static
void	callback_fibQuality (bool b, int16_t snr, int32_t offs, void *ctx) {
PyObject *arglist;
PyObject *result;
PyGILState_STATE gstate;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("bhiO", b, snr, offs, ctx);
	result  = PyEval_CallObject (callbackFibQuality, arglist);
//	Py_DECREF (arglist);
	PyGILState_Release (gstate);
	(void)result;
}
//	typedef	void (*programQuality_t)(int16_t,
//	                                 int16_t,
//	                                 int16_t,
//	                                 void *);
//
PyObject *callbackProgramQuality	= NULL;
static
void	callback_programQuality (int16_t c1, int16_t c2, int16_t c3, void *ctx) {
PyObject *arglist;
PyObject *result;
PyGILState_STATE gstate;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("hhhO", c1, c2, c3, ctx);
	result  = PyEval_CallObject (callbackProgramQuality, arglist);
//	Py_DECREF (arglist);
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
//	                   programdata_t       programdataHandler,
//	                   programQuality_t    program_qualityHandler,
//	                   void                *userData);

PyObject *dabInit_P (PyObject *self, PyObject *args) {
int	theMode	= 127;
PyObject	*deviceH;
PyObject	*cbsH;		// callback for syncsignalHandler
PyObject	*cbsD;		// callback for systemdataHandler
PyObject	*cbeN;		// callback for ensemble name
PyObject	*cbpN;		// callback for program name
PyObject	*cbfQ;		// callback for fib quality
PyObject	*cbaO;		// callback for audioOut
PyObject	*cbdO;		// callback for dataOut
PyObject	*cbpQ;		// callback for program Quality

PyObject	*d1;
PyObject	*d2;
void	*result;
//
	(void)PyArg_ParseTuple (args,
	                      "OhOOOOOOOO",
	                       &deviceH,
	                       &theMode,
	                       &cbsH,
	                       &cbsD,
	                       &cbeN,
	                       &cbpN,
	                       &cbfQ,
	                       &cbaO,
	                       &cbdO,
	                       &cbpQ
	                     );
	(void)d1;
	(void)d2;
//
//	The callbacks should be callable

	if (!PyCallable_Check (cbsH)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for audio must be callable");
	   Py_RETURN_NONE;
	}

	if (!PyCallable_Check (cbsD)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for data must be callable");
	   Py_RETURN_NONE;
	}

	if (!PyCallable_Check (cbeN)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for ensemble must be callable");
	   Py_RETURN_NONE;
	}

	if (!PyCallable_Check (cbpN)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for program data must be callable");
	   Py_RETURN_NONE;
	}

	if (!PyCallable_Check (cbfQ)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for ensemble must be callable");
	   Py_RETURN_NONE;
	}

	if (!PyCallable_Check (cbaO)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for program data must be callable");
	   Py_RETURN_NONE;
	}

	if (!PyCallable_Check (cbdO)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for ensemble must be callable");
	   Py_RETURN_NONE;
	}

	if (!PyCallable_Check (cbpQ)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for program data must be callable");
	   Py_RETURN_NONE;
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

	Py_XINCREF (cbpQ);
	if (callbackProgramQuality != NULL)
	   Py_XDECREF (callbackProgramQuality);
	callbackProgramQuality	= cbpQ;

	result = dabInit ((deviceHandler *)deviceH,
	                  theMode,
	                  NULL,
	                  NULL,
	                  (syncsignal_t)callbackSyncSignal,
	                  (systemdata_t)callbackSystemData,
	                  (ensemblename_t)callbackEnsembleName,
	                  (programname_t)callbackProgramName,
	                  (fib_quality_t)callbackFibQuality,
	                  (audioOut_t)callbackAudioOut,
	                  (dataOut_t)callbackDataOut,
	                  NULL,
	                  (programQuality_t)callbackProgramQuality,
	                  NULL
	                 );
	if (result == NULL) {
	   fprintf (stderr, "lukte toch niet\n");
	   Py_RETURN_NONE;
	}
	return PyCapsule_New (result, "library_object", NULL);
}

PyObject *dabExit_P (PyObject *self, PyObject *args) {
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

	PyArg_ParseTuple (args, "O", &handle_capsule);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	dabStartProcessing (handle);
	Py_INCREF (Py_None);
	Py_RETURN_NONE;
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
	Py_INCREF (Py_None);
	Py_RETURN_NONE;
}

//	bool	dab_Service	(std::string, void *handle);
PyObject *dabService_p (PyObject *self, PyObject *args) {
PyObject	*handle_capsule;
void		*handle;
char		*s;

	PyArg_ParseTuple (args, "sO", &s, &handle_capsule);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	dabService (s, handle);
	Py_RETURN_NONE;
}

static PyMethodDef module_methods [] = {
	{"dabInit",	dabInit_p,	METH_VARARGS, ""},
	{"dabExit",	dabExit_p,	METH_VARARGS, ""},
	{"dabReset",	dabReset_p,	METH_VARARGS, ""},
	{"dabStop",	dabStop_p,	METH_VARARGS, ""},
	{"dabService",	dabService_p,	METH_VARARGS, ""},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef dablib = {
	PyModuleDef_HEAD_INIT,
	"dab_python",
	"",
	-1,
	module_methods,
	NULL,
	NULL,
	NULL,
	NULL
};


PyObject *PyInit_libdab_lib (void) {
	if (!PyEval_ThreadsInitialized ())
	   PyEval_InitThreads ();
	import_array ();
	return PyModule_Create (&dablib);
}


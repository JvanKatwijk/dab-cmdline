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
PyObject *dabInit_p	(PyObject *self, PyObject *args);
PyObject *dabExit_p		(PyObject *self, PyObject *args);
PyObject *dabReset_p	(PyObject *self, PyObject *args);
PyObject *dabService_p		(PyObject *self, PyObject *args);
PyObject *dabgetSid_p		(PyObject *self, PyObject *args);
PyObject *dab_stop_p		(PyObject *self, PyObject *args);
PyObject *PyInit_libdab_lib	(void);
}
//
//	We need to define 'C' callback functions, whose purpose
//	is to call the appropriate Python function
//
//	The list of strings - if any - is packed ito a tuple
//	and passed as parameter.
//	typedef void (*cb_ensemble_t)(std::list<std::string>, bool);
static
PyObject *callbackEnsembleInfo	= NULL;
static
void	callback_ensemble (std::list<std::string> s, bool b) {
PyGILState_STATE gstate;
PyObject *arglist;
PyObject *result;
PyObject *stringTuple = PyTuple_New (Py_ssize_t (s. size ()));
int16_t	next	= 0;

	fprintf (stderr, "we zijn in callback_ensemble");
	for (std::list<std::string>::iterator list_iter = s. begin ();
           list_iter != s. end (); list_iter ++) {
	   PyObject *temp = Py_BuildValue ("s", (*list_iter).c_str ());
	   PyTuple_SetItem (stringTuple, next++, temp);
        }
	
	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("(Oh)", stringTuple, b);
	result  = PyEval_CallObject (callbackEnsembleInfo, arglist);
	Py_DECREF (arglist);
	PyGILState_Release (gstate);
}


PyObject *callbackAudio	= NULL;
//	typedef void (*cb_audio_t)(int16_t *, int, int);
static
void	callback_audio (int16_t *b, int size, int rate) {
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
	arglist = Py_BuildValue ("Ohh", theArray, size / 2, rate);
	result  = PyEval_CallObject (callbackAudio, arglist);
	Py_DECREF (arglist);
	Py_DECREF (theArray);
	PyGILState_Release (gstate);
}

PyObject *callbackData	= NULL;
//	typedef void (*cb_data_t)(std::string);
static
void	callback_data (std::string str) {
PyObject *arglist;
PyObject *result;
PyGILState_STATE gstate;

	gstate	= PyGILState_Ensure ();
	arglist = Py_BuildValue ("(z)", str. c_str ());
	result  = PyEval_CallObject (callbackData, arglist);
//	Py_DECREF (arglist);
	PyGILState_Release (gstate);
}

PyObject	* callbackProgramInfo	= NULL;
//	typedef void (*cb_programdata_t)(int16_t,	// start address
//	                                 int16_t,	// length
//	                                 int16_t,	// subchId
//	                                 int16_t,	// bitRate
//	                                 int16_t	// protection
//	                                );
static
void	callback_programdata (int16_t startAddress,
	                      int16_t length,
	                      int16_t subchId,
	                      int16_t bitRate,
	                      int16_t protection) {
PyGILState_STATE gstate;
PyObject	*arglist;
PyObject	*result;

	gstate	= PyGILState_Ensure ();
	arglist	= Py_BuildValue ("hhhhh", startAddress,
	                                  length,
	                                  subchId,
	                                  protection,
	                                  bitRate);
	
	result  = PyEval_CallObject (callbackProgramInfo, arglist);
	Py_DECREF (arglist);
	PyGILState_Release (gstate);
}
//
//	Here the real API starts
//
//	waiting time i an undocumented parameter, indicating the
//	time we will wait before deciding we have an ensemble
//
//	void	*dab_initialize	(void	*,
//	                         uint8_t,	// dab Mode
//	                         dabBand,	// Band
//	                         int16_t, 	// waitingTime = 10,
//	                         cb_audio_t,	// callback for sound output
//	                         cb_data_t,	// callback for dynamic labels
//	                         cb_systemdata_t,
//	                         cb_fib_quality_t,
//	                         cb_msc_quality_t,
//	                         cb_ensemble_t,
//	                         cb_programdata_t
//	                         );
PyObject *dab_initialize_p (PyObject *self, PyObject *args) {
int	theMode	= 127;
int	theBand	= 127;
PyObject *cba;		// callback for audio out
PyObject *cbd;		// callback for dynamic label out
PyObject *cbe;		// callback for ensemble info
PyObject *cbp;		// callback for program data
int16_t	delayTime;
void	*result;
int	r;
//
	r = PyArg_ParseTuple (args,
	                      "hhhOOOO",
	                       &theMode,
	                       &theBand,
	                       &delayTime,
	                       &cba,
	                       &cbd,
	                       &cbe,
	                       &cbp
	                  );
//
//	The callbacks should be callable

	if (!PyCallable_Check (cba)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for audio must be callable");
	   Py_RETURN_NONE;
	}

	if (!PyCallable_Check (cbd)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for data must be callable");
	   Py_RETURN_NONE;
	}

	if (!PyCallable_Check (cbe)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for ensemble must be callable");
	   Py_RETURN_NONE;
	}

	if (!PyCallable_Check (cbp)) {
	   PyErr_SetString(PyExc_TypeError, "parameter for program data must be callable");
	   Py_RETURN_NONE;
	}

	Py_XINCREF (cba);
	if (callbackAudio != NULL)
	   Py_XDECREF (callbackAudio);
	callbackAudio	= cba;

	Py_XINCREF (cbd);
	if (callbackData != NULL)
	   Py_XDECREF (callbackData);
	callbackData	= cbd;

	Py_XINCREF (cbe);
	if (callbackEnsembleInfo != NULL)
	   Py_XDECREF (callbackEnsembleInfo);
	callbackEnsembleInfo	= cbe;

	Py_XINCREF (cbp);
	if (callbackProgramInfo != NULL)
	   Py_XDECREF (callbackProgramInfo);
	callbackProgramInfo	= cbp;

	result = dab_initialize (NULL,
	                         theMode,
	                         (dabBand)theBand,
	                         delayTime,
	                         (cb_audio_t)&callback_audio,
	                         (cb_data_t)&callback_data,
	                         NULL,		// systemdata
	                         NULL,		// fibQuality
	                         NULL,		// mscQuality
	                         (cb_ensemble_t)&callback_ensemble,
	                         (cb_programdata_t)&callback_programdata
	                        );
	if (result == NULL) {
	   fprintf (stderr, "lukte toch niet\n");
	   Py_RETURN_NONE;
	}
	return PyCapsule_New (result, "library_object", NULL);
}

//	void	dab_Gain (void *, uint16_t);
PyObject *dab_Gain_p (PyObject *self, PyObject *args) {
PyObject *handle_capsule;
void	*handle;
int	gain;

	PyArg_ParseTuple (args, "Oh", &handle_capsule, &gain);
	fprintf (stderr, "the gain will be set to %d\n", gain);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	dab_Gain (handle, gain);
	Py_INCREF (Py_None);
	Py_RETURN_NONE;
}

//	bool	dab_Channel	(void *handle, std::string);
PyObject *dab_channel_p (PyObject *self, PyObject *args) {
PyObject *handle_capsule;
void	*handle;
char	*s;
bool	b;

	PyArg_ParseTuple (args, "Os", &handle_capsule, &s);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	b = dab_Channel (handle, s);
	return Py_BuildValue ("b", b);
}

//	void	dab_run		(void *handle);
PyObject *dab_run_p (PyObject *self, PyObject *args) {
PyObject	*handle_capsule;
void		*handle;

	PyArg_ParseTuple (args, "O", &handle_capsule);

	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	dab_run (handle);
	Py_INCREF (Py_None);
	Py_RETURN_NONE;
}

//	bool	dab_Service	(void *handle, std::string);
PyObject *dab_Service_p (PyObject *self, PyObject *args) {
PyObject	*handle_capsule;
void		*handle;
char		*s;
int16_t		localArray [1024];
npy_intp 	dims [1];

	PyArg_ParseTuple (args, "Os", &handle_capsule, &s);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	dab_Service (handle, s);
	Py_RETURN_NONE;
}

//	bool	dab_Service_Id	(void *handle, int32_t);
PyObject *dab_Service_Id_p (PyObject *self, PyObject *args) {
PyObject	*handle_capsule;
void		*handle;
int		serviceId;

	PyArg_ParseTuple (args, "Oi", &handle_capsule, &serviceId);

	fprintf (stdout, "serviceId = %d\n", serviceId);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	dab_Service_Id (handle, serviceId);
	Py_RETURN_NONE;
}

//	void	dab_stop	(void *handle);
PyObject *dab_stop_p	(PyObject *self, PyObject *args) {
PyObject *handle_capsule;
void	*handle;
	PyArg_ParseTuple (args, "O", &handle_capsule);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	dab_stop (handle);
	Py_RETURN_NONE;
}

//	void	dab_exit	(void *handle);
PyObject *dab_exit_p	(PyObject *self, PyObject *args) {
PyObject *handle_capsule;
void	*handle;

	PyArg_ParseTuple (args, "O", &handle_capsule);
	handle = PyCapsule_GetPointer (handle_capsule, "library_object");
	dab_exit (&handle);
	Py_RETURN_NONE;
}


static PyMethodDef module_methods [] = {
	{"dab_initialize", dab_initialize_p, METH_VARARGS, ""},
	{"dab_Gain",       dab_Gain_p,       METH_VARARGS, ""},
	{"dab_channel",    dab_channel_p,    METH_VARARGS, ""},
	{"dab_run",        dab_run_p,        METH_VARARGS, ""},
	{"dab_Service",    dab_Service_p,    METH_VARARGS, ""},
	{"dab_Service_Id", dab_Service_Id_p, METH_VARARGS, ""},
	{"dab_stop",       dab_stop_p,       METH_VARARGS, ""},
	{"dab_exit",       dab_exit_p,       METH_VARARGS, ""},
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


#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming 
 *
 *    This file is part of the DAB-library
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
 *    along with DAB-library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
//
//	dummy for the dab handler
//
#include	"dab-constants.h"
#include	"virtual-backend.h"

        virtualBackend::virtualBackend  (int16_t a, int16_t l) {
        startAddress    = a;
        segmentLength   = l;
}

        virtualBackend::~virtualBackend (void) {
}

int32_t virtualBackend::process (int16_t *v, int16_t c) {
        (void)v;
        (void)c;
        return 32768;
}

int16_t virtualBackend::startAddr (void) {
        return startAddress;
}

int16_t virtualBackend::Length (void) {
        return segmentLength;
}


void    virtualBackend::stopRunning     (void) {
}

void    virtualBackend::stop    (void) {
}



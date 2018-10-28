#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<stdbool.h>
#include	<unistd.h>
#include	<sys/socket.h>
#include	<bluetooth/bluetooth.h>
#include	<bluetooth/hci.h>
#include	<bluetooth/hci_lib.h>
#include	<bluetooth/rfcomm.h>
#include	<bluetooth/sdp.h>
#include	<bluetooth/sdp_lib.h>

#include	"bluetooth.h"
//
//	the simple C wrapper 
static
char	name [248];
static
bool	checkService (const bdaddr_t *target);

static
bool	getDevice  (bdaddr_t *target) {
inquiry_info *ii = NULL;
int max_rsp, num_rsp;
int dev_id, sock, len, flags;
int i;
char addr [19] = { 0 };

	memset (name, 0, 248 * sizeof (char));
	dev_id = hci_get_route (NULL);
	sock = hci_open_dev (dev_id );
	if (dev_id < 0 || sock < 0) {
	   perror ("opening socket");
	   exit(1);
	}

	len  = 8;
	max_rsp = 255;
	flags = IREQ_CACHE_FLUSH;
	ii = (inquiry_info*)malloc (max_rsp * sizeof(inquiry_info));
    
	num_rsp = hci_inquiry (dev_id, len, max_rsp, NULL, &ii, flags);
	if (num_rsp < 0)
	   perror ("hci_inquiry");

	for (i = 0; i < num_rsp; i++) {
	   ba2str (&(ii + i) -> bdaddr, addr);
	   memset (name, 0, sizeof (name));
	   if (hci_read_remote_name (sock, &(ii + i)->bdaddr,
	                             sizeof (name), 
	                             name, 0) < 0)
	      strcpy (name, "[unknown]");
	   fprintf (stderr, "potential server with name %s (%s) found\n",
	                                            name, addr);

	   if (checkService (&(ii + i) -> bdaddr)) {
	      *target = (ii + i) -> bdaddr;
	      free (ii);
	      close (sock);
	      return true;
	   }
	   else
	      printf ("%s  %s does not provide the service\n", addr, name);
	}

	free (ii);
	close (sock);
	return false;
}

bool	checkService (const bdaddr_t *target) {
uint8_t svc_uuid_int[] =
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xab, 0xcd };
uuid_t svc_uuid;
int err;
sdp_list_t *response_list = NULL,
	   *search_list,
	   *attrid_list;
sdp_session_t *session = 0;

//	connect to the SDP server running on the remote machine
	session		= sdp_connect (BDADDR_ANY,
	                               target,
	                               SDP_RETRY_IF_BUSY);

	if (session == NULL) {
	   fprintf (stderr, "could not connect \n");
	   return false;
	}

//	specify the UUID of the application we're searching for
	sdp_uuid128_create (&svc_uuid, &svc_uuid_int);
	search_list	= sdp_list_append (NULL, &svc_uuid);

//	specify that we want a list of all the matching applications' attributes
	uint32_t range	= 0x0000ffff;
	attrid_list	= sdp_list_append (NULL, &range);

//	get a list of service records that have UUID 0xabcd
	err		= sdp_service_search_attr_req (session,
	                                               search_list, 
	                                               SDP_ATTR_REQ_RANGE,
	                                               attrid_list,
	                                               &response_list);

	sdp_list_t *r	= response_list;

//	go through each of the service records
	for (; r; r = r -> next) {
	   sdp_record_t *rec = (sdp_record_t*) r -> data;
	   sdp_list_t *proto_list;
        
//	get a list of the protocol sequences
	   if (sdp_get_access_protos (rec, &proto_list) == 0 ) {
	      sdp_list_t *p = proto_list;

//	go through each protocol sequence
	      for (; p; p = p -> next ) {
	         sdp_list_t *pds = (sdp_list_t *)p -> data;

//	go through each protocol list of the protocol sequence
	         for (; pds; pds = pds -> next) {

//	check the protocol attributes
	            sdp_data_t *d = (sdp_data_t *)pds -> data;
	            int proto = 0;
	            for (; d; d = d -> next) {
	               switch (d -> dtd) { 
	                  case SDP_UUID16:
	                  case SDP_UUID32:
	                  case SDP_UUID128:
	                     proto = sdp_uuid_to_proto (&d -> val. uuid);
	                     break;

	                  case SDP_UINT8:
	                     if (proto == RFCOMM_UUID) {
	                        fprintf (stderr, 
	                                 "rfcomm channel: %d\n",
	                                  d -> val. int8);
	                     }
//	                     break;
	                     sdp_list_free ((sdp_list_t *)p -> data, 0);
	                     sdp_list_free (proto_list, 0);
	                     sdp_record_free (rec);
	                     sdp_close (session);
	                     return true;
	               }
	            }
	         }

	         sdp_list_free ((sdp_list_t *)p -> data, 0);
	      }
	      sdp_list_free (proto_list, 0);
	   }	// end of "if" statement

//	   fprintf (stderr, "found service record 0x%x\n", rec -> handle);
	   sdp_record_free (rec);
	}

	sdp_close(session);
	return false;
}

int	theSocket;
bool	bluetooth_connect (void) {
bdaddr_t theDevice;
bool	result	= getDevice (&theDevice);
char	address [255];
char	buffer [255];
struct sockaddr_rc addr = { 0 };
int s, status;
char dest [18] = "B8:27:EB:28:C4:41";

	if (!result) {
	   fprintf (stderr, "No server for our service found, quitting\n");
	   return false;
	}
	ba2str (&theDevice, address);
	fprintf (stderr, "we hebben een match, %s\n", address);

//      allocate a socket
        theSocket = socket (AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

//      set the connection parameters (who to connect to)
        addr. rc_family = AF_BLUETOOTH;
        addr. rc_channel = (uint8_t) 1;
        str2ba (address, &addr. rc_bdaddr);

//      connect to server
        status = connect (theSocket, (struct sockaddr *)&addr, sizeof(addr));
	fprintf (stderr, "status = %d\n", status);
	return true;
}

int	bluetooth_read (char *buffer, int maxsize) {
	return read (theSocket, buffer, maxsize);
}

int	bluetooth_write (char *buffer, int size) {
	return write (theSocket, buffer, size);
}

void	bluetooth_terminate	(void) {
	close (theSocket);
}


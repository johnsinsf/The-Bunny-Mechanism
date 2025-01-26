/*
 * bunny.h : bunny-uShare Web Server handler.
 * Copyright (C) 2025 John Sullivan <john@dwanta.com>
 *
 * original header:
 * Originally developped for the GeeXboX project.
 * Parts of the code are originated from GMediaServer from Oskar Liljeblad.
 * Copyright (C) 2005-2007 Benjamin Zores <ben@geexbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _BUNNY_H_
#define _BUNNY_H_

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include "Mech.h"
#include "LObj.h"

int bunny_get_info (const char *filename, UpnpFileInfo *info,
	const void* cookie __attribute__((unused)),
	const void** requestCookie __attribute__((unused)));

UpnpWebFileHandle bunny_open (const char *filename, enum UpnpOpenFileMode mode,
	const void* cookie __attribute__((unused)),
	const void* requestCookie __attribute__((unused)));

int bunny_read (UpnpWebFileHandle fh, char *buf, size_t buflen,
	const void* cookie __attribute__((unused)),
	const void* requestCookie __attribute__((unused)));

int bunny_seek (UpnpWebFileHandle fh, off_t offset, int origin,
	const void* cookie __attribute__((unused)),
	const void* requestCookie __attribute__((unused)));

int bunny_write (UpnpWebFileHandle fh __attribute__((unused)),
	char *buf __attribute__((unused)),
	size_t buflen __attribute__((unused)),
	const void* cookie __attribute__((unused)),
	const void* requestCookie __attribute__((unused)));

int bunny_close (UpnpWebFileHandle fh,
 	const void* cookie __attribute__((unused)),
	const void* requestCookie __attribute__((unused)));

int bunny_server_connect(UpnpWebFileHandle fh);

int readHeader( SocketIO* socket, LObj& lobj );
int parseHeader( SocketIO* socket, LObj& lobj );

#endif /* _BUNNY_H_ */

/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
 * Free Software Foundation.
 *
 * Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
 * GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
 * A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
 * obter mais detalhes.
 *
 * Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
 * programa; se não, escreva para a Free Software Foundation, Inc., 51 Franklin
 * St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Este programa está nomeado como rpqtest.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */


#include <stdio.h>
#include <string.h>

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <3270ds.h>

#include "globals.h"
#include <sf.h>
#include <ctlrc.h>

static int dump_buffer(H3270 *hSession, unsigned const char *buf, int len)
{
	return 0;
}

int main(int numpar, char *param[])
{
	unsigned char buffer[] = { 0xf3, 0x00, 0x05, 0x01, 0xff, 0x02 };

	H3270 *hSession = lib3270_session_new("");
	hSession->write = dump_buffer;

	lib3270_set_toggle(hSession,LIB3270_TOGGLE_DS_TRACE,1);

	write_structured_field(hSession, buffer, 6);

	lib3270_session_free(hSession);
	return 0;
}

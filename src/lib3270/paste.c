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
 * Este programa está nomeado como paste.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 * macmiranda@bb.com.br		(Marco Aurélio Caldas Miranda)
 *
 */

#include "private.h"

/*
#if defined(X3270_DISPLAY)
	#include <X11/Xatom.h>
#endif
*/

#define XK_3270

#if defined(X3270_APL)
	#define XK_APL
#endif

#include <fcntl.h>

#include "3270ds.h"
#include "resources.h"

//#include "actionsc.h"
#include "ansic.h"
//#include "aplc.h"
#include "ctlrc.h"
#include "ftc.h"
#include "hostc.h"
// #include "keypadc.h"
#include "kybdc.h"
#include "popupsc.h"
// #include "printc.h"
#include "screenc.h"

/*
#if defined(X3270_DISPLAY)
	#include "selectc.h"
#endif
*/

#include "statusc.h"
// #include "tablesc.h"
#include "telnetc.h"
#include "togglesc.h"
#include "trace_dsc.h"
#include "utf8c.h"
#include "utilc.h"
#if defined(X3270_DBCS) /*[*/
#include "widec.h"
#endif /*]*/
#include "api.h"

#include <lib3270/popup.h>
#include <lib3270/selection.h>

/*---[ Struct ]-------------------------------------------------------------------------------------------------*/

 typedef struct _paste_data
 {
 	int qtd;
 	int row;
	int orig_addr;
	int orig_col;
 } PASTE_DATA;

/*---[ Implement ]----------------------------------------------------------------------------------------------*/

/*
 * Move the cursor back within the legal paste area.
 * Returns a Boolean indicating success.
 */
 static int remargin(H3270 *hSession, int lmargin)
 {
	int ever = False;
	int baddr, b0 = 0;
	int faddr;
	unsigned char fa;

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_MARGINED_PASTE))
	{
		baddr = hSession->cursor_addr;
		while(BA_TO_COL(baddr) < lmargin)
		{
			baddr = ROWCOL_TO_BA(BA_TO_ROW(baddr), lmargin);
			if (!ever)
			{
				b0 = baddr;
				ever = True;
			}

			faddr = find_field_attribute(hSession,baddr);
			fa = hSession->ea_buf[faddr].fa;
			if (faddr == baddr || FA_IS_PROTECTED(fa))
			{
				baddr = lib3270_get_next_unprotected(hSession,baddr);
				if (baddr <= b0)
					return 0;
			}

		}
		cursor_move(hSession,baddr);
	}

	return -1;
 }

 static int paste_char(H3270 *hSession, PASTE_DATA *data, unsigned char c)
 {

	if(lib3270_get_toggle(hSession,LIB3270_TOGGLE_SMART_PASTE))
	{
		int faddr = find_field_attribute(hSession,hSession->cursor_addr);
		if(FA_IS_PROTECTED(hSession->ea_buf[faddr].fa))
			hSession->cursor_addr++;
		else
			key_ACharacter(hSession, c, KT_STD, IA_PASTE, NULL);
	}
	else
	{
		key_ACharacter(hSession, c, KT_STD, IA_PASTE, NULL);
	}

	data->qtd++;

 	if(BA_TO_ROW(hSession->cursor_addr) != data->row)
 	{
 		trace("Row changed from %d to %d",data->row,BA_TO_ROW(hSession->cursor_addr));
		if(!remargin(hSession,data->orig_col))
			return 0;
		data->row = BA_TO_ROW(hSession->cursor_addr);
 		return '\n';
 	}

 	return c;
}

static int set_string(H3270 *hSession, const unsigned char *str)
{
	PASTE_DATA data;
	unsigned char last = 1;

	memset(&data,0,sizeof(data));
 	data.row		= BA_TO_ROW(hSession->cursor_addr);
	data.orig_addr	= hSession->cursor_addr;
	data.orig_col	= BA_TO_COL(hSession->cursor_addr);

	while(*str && last && !hSession->kybdlock && hSession->cursor_addr >= data.orig_addr)
	{
		switch(*str)
		{
		case '\t':
			last = paste_char(hSession,&data, ' ');
			break;

		case '\n':
			if(last != '\n')
			{
				int baddr;
				int faddr;
				unsigned char fa;

				baddr = (hSession->cursor_addr + hSession->cols) % (hSession->cols * hSession->rows);   /* down */
				baddr = (baddr / hSession->cols) * hSession->cols;               /* 1st col */
				faddr = find_field_attribute(hSession,baddr);
				fa = hSession->ea_buf[faddr].fa;
				if (faddr != baddr && !FA_IS_PROTECTED(fa))
					cursor_move(hSession,baddr);
				else
					cursor_move(hSession,lib3270_get_next_unprotected(hSession,baddr));
				data.row = BA_TO_ROW(hSession->cursor_addr);
			}
			last = ' ';
			data.qtd++;
			break;

		default:
			last = paste_char(hSession,&data, *str);

		}
		str++;

		if(IN_3270 && lib3270_get_toggle(hSession,LIB3270_TOGGLE_MARGINED_PASTE) && BA_TO_COL(hSession->cursor_addr) < data.orig_col)
		{
			if(!remargin(hSession,data.orig_col))
				last = 0;
		}

		if(hSession->cursor_addr == data.orig_addr)
			break;
	}

	return data.qtd;
}

LIB3270_EXPORT int lib3270_set_string_at(H3270 *hSession, int row, int col, const unsigned char *str)
{
    int rc = -1;

	CHECK_SESSION_HANDLE(hSession);

	if(hSession->kybdlock)
		return -EINVAL;

	if(hSession->selected && !lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_SELECTED))
		lib3270_unselect(hSession);

	row--;
	col--;

	if(row >= 0 && col >= 0 && row <= hSession->rows && col <= hSession->cols)
	{
		hSession->cbk.suspend(hSession);

		hSession->cursor_addr = (row * hSession->cols) + col;
		rc = set_string(hSession, str);

		hSession->cbk.resume(hSession);
	}

	return rc;
}


/**
 * Set string at cursor position.
 *
 * @param hSession		Session handle.
 * @param str			String to set
 *
 * @return Number of characters inserted; <0 in case of error.
 *
 */
LIB3270_EXPORT int lib3270_set_string(H3270 *hSession, const unsigned char *str)
{
	int rc;

	CHECK_SESSION_HANDLE(hSession);

	if(hSession->kybdlock)
		return -EINVAL;

	hSession->cbk.suspend(hSession);
	rc = set_string(hSession, str);
	hSession->cbk.resume(hSession);

	return rc;
}

LIB3270_EXPORT int lib3270_paste(H3270 *h, const unsigned char *str)
{
	int sz;
	CHECK_SESSION_HANDLE(h);

	if(!lib3270_connected(h))
	{
		lib3270_ring_bell(h);
		return 0;
	}

	if(h->paste_buffer)
	{
		lib3270_free(h->paste_buffer);
		h->paste_buffer = NULL;
	}

	sz = lib3270_set_string(h,str);
	if(sz < 0)
	{
		// Can´t paste
		lib3270_popup_dialog(h,LIB3270_NOTIFY_WARNING,
										_( "Action failed" ),
										_( "Unable to paste text" ),
										"%s", sz == -EINVAL ? _( "Keyboard is locked" ) : _( "Unexpected error" ) );
		return 0;
	}

	if(strlen((char *) str) > sz)
	{
		h->paste_buffer = strdup((char *) (str+sz));
		return 1;
	}

	return 0;
}

LIB3270_ACTION(pastenext)
{
	char	* ptr;
	int		  rc;

	CHECK_SESSION_HANDLE(hSession);

	if(!(lib3270_connected(hSession) && hSession->paste_buffer))
	{
		lib3270_ring_bell(hSession);
		return 0;
	}

	ptr = hSession->paste_buffer;
	hSession->paste_buffer = NULL;

	rc = lib3270_paste(hSession,(unsigned char *) ptr);

	lib3270_free(ptr);
	return rc;
}

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
 * Este programa está nomeado como host.c e possui 1078 linhas de código.
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


/*
 *	host.c
 *		This module handles the ibm_hosts file, connecting to and
 *		disconnecting from hosts, and state changes on the host
 *		connection.
 */

#include <malloc.h>
#include "private.h"
// #include "appres.h"
#include "resources.h"

//#include "actionsc.h"
#include "hostc.h"
#include "statusc.h"
#include "popupsc.h"
#include "telnetc.h"
#include "trace_dsc.h"
#include "utilc.h"
#include "xioc.h"

#include <errno.h>
#include <lib3270/internals.h>

#define RECONNECT_MS		2000	/* 2 sec before reconnecting to host */
#define RECONNECT_ERR_MS	5000	/* 5 sec before reconnecting to host */

static void try_reconnect(H3270 *session);

/*
 * Called from timer to attempt an automatic reconnection.
 */
static void try_reconnect(H3270 *session)
{
	lib3270_write_log(session,"3270","Starting auto-reconnect (Host: %s)",session->host.full ? session->host.full : "-");
	session->auto_reconnect_inprogress = 0;
	lib3270_connect(session,0);
}

LIB3270_EXPORT int lib3270_disconnect(H3270 *h)
{
	host_disconnect(h,0);
	return 0;
}

void host_disconnect(H3270 *hSession, int failed)
{
    CHECK_SESSION_HANDLE(hSession);

	if (CONNECTED || HALF_CONNECTED)
	{
		// Disconecting, disable input
		remove_input_calls(hSession);
		net_disconnect(hSession);

		trace("Disconnected (Failed: %d Reconnect: %d in_progress: %d)",failed,lib3270_get_toggle(hSession,LIB3270_TOGGLE_RECONNECT),hSession->auto_reconnect_inprogress);
		if (lib3270_get_toggle(hSession,LIB3270_TOGGLE_RECONNECT) && !hSession->auto_reconnect_inprogress)
		{
			/* Schedule an automatic reconnection. */
			hSession->auto_reconnect_inprogress = 1;
			(void) AddTimeOut(failed ? RECONNECT_ERR_MS: RECONNECT_MS, hSession, try_reconnect);
		}

		/*
		 * Remember a disconnect from ANSI mode, to keep screen tracing
		 * in sync.
		 */
#if defined(X3270_TRACE) /*[*/
		if (IN_ANSI && lib3270_get_toggle(hSession,LIB3270_TOGGLE_SCREEN_TRACE))
			trace_ansi_disc(hSession);
#endif /*]*/

		lib3270_set_disconnected(hSession);
	}
}

/* The host has entered 3270 or ANSI mode, or switched between them. */
void host_in3270(H3270 *hSession, LIB3270_CSTATE new_cstate)
{
	Boolean now3270 = (new_cstate == LIB3270_CONNECTED_3270 ||
			   new_cstate == LIB3270_CONNECTED_SSCP ||
			   new_cstate == LIB3270_CONNECTED_TN3270E);

	hSession->cstate = new_cstate;
	hSession->ever_3270 = now3270;
	lib3270_st_changed(hSession, LIB3270_STATE_3270_MODE, now3270);
}

void lib3270_set_connected(H3270 *hSession)
{
	hSession->cstate	= LIB3270_CONNECTED_INITIAL;
	hSession->starting	= 1;	// Enable autostart

	lib3270_st_changed(hSession, LIB3270_STATE_CONNECT, True);
	if(hSession->cbk.update_connect)
		hSession->cbk.update_connect(hSession,1);
}

void lib3270_set_disconnected(H3270 *hSession)
{
	CHECK_SESSION_HANDLE(hSession);

	hSession->cstate	= LIB3270_NOT_CONNECTED;
	hSession->starting	= 0;
	hSession->secure	= LIB3270_SSL_UNDEFINED;

	set_status(hSession,OIA_FLAG_UNDERA,False);

	lib3270_st_changed(hSession,LIB3270_STATE_CONNECT, False);

	status_changed(hSession,LIB3270_MESSAGE_DISCONNECTED);

	if(hSession->cbk.update_connect)
		hSession->cbk.update_connect(hSession,0);

	hSession->cbk.update_ssl(hSession,hSession->secure);

}

/* Register a function interested in a state change. */
LIB3270_EXPORT void lib3270_register_schange(H3270 *h, LIB3270_STATE tx, void (*func)(H3270 *, int, void *),void *data)
{
	struct lib3270_state_callback *st;

    CHECK_SESSION_HANDLE(h);

	st 			= (struct lib3270_state_callback *) lib3270_malloc(sizeof(struct lib3270_state_callback));
	st->func	= func;

	if (h->st_last[tx])
		h->st_last[tx]->next = st;
	else
		h->st_callbacks[tx] = st;

	h->st_last[tx] = st;

}

/* Signal a state change. */
void lib3270_st_changed(H3270 *h, LIB3270_STATE tx, int mode)
{
#if defined(DEBUG)

	static const char * state_name[LIB3270_STATE_USER] =
	{
		"LIB3270_STATE_RESOLVING",
		"LIB3270_STATE_HALF_CONNECT",
		"LIB3270_STATE_CONNECT",
		"LIB3270_STATE_3270_MODE",
		"LIB3270_STATE_LINE_MODE",
		"LIB3270_STATE_REMODEL",
		"LIB3270_STATE_PRINTER",
		"LIB3270_STATE_EXITING",
		"LIB3270_STATE_CHARSET",

	};

#endif // DEBUG

	struct lib3270_state_callback *st;

    CHECK_SESSION_HANDLE(h);

	trace("%s is %d on session %p",state_name[tx],mode,h);

	for (st = h->st_callbacks[tx];st;st = st->next)
	{
//		trace("st=%p func=%p",st,st->func);
		st->func(h,mode,st->data);
	}

	trace("%s ends",__FUNCTION__);
}

static void update_host(H3270 *h)
{
	Replace(h->host.full,
			lib3270_strdup_printf(
				"%s%s:%s",
					h->options&LIB3270_OPTION_SSL ? "tn3270s://" : "tn3270://",
					h->host.current,
					h->host.srvc
		));

	trace("hosturl=[%s] ssl=%s",h->host.full,(h->options&LIB3270_OPTION_SSL) ? "yes" : "no");

}

LIB3270_EXPORT const char * lib3270_get_url(H3270 *h, char *buffer, int len)
{
	CHECK_SESSION_HANDLE(h);

	snprintf(buffer,len,"%s://%s:%s",
			((h->options & LIB3270_OPTION_SSL) == 0) ? "tn3270" : "tn3270s",
			h->host.current,
			h->host.srvc
	);

	return buffer;
}

LIB3270_EXPORT const char * lib3270_set_url(H3270 *h, const char *n)
{
    CHECK_SESSION_HANDLE(h);

	if(n && n != h->host.full)
	{
		static const struct _sch
		{
			LIB3270_OPTION	  opt;
			const char		* text;
			const char		* srvc;
		} sch[] =
		{
			{ LIB3270_OPTION_DEFAULTS,  "tn3270://",	"telnet"	},
			{ LIB3270_OPTION_SSL,		"tn3270s://",	"telnets"	},
			{ LIB3270_OPTION_DEFAULTS,  "telnet://",	"telnet"	},
			{ LIB3270_OPTION_DEFAULTS,  "telnets://",	"telnets"	},
			{ LIB3270_OPTION_SSL,		"L://",			"telnets"	},

			{ LIB3270_OPTION_SSL,		"L:",			"telnets"	}	// The compatibility should be the last option
		};

		char					* str 		= strdup(n);
		char					* hostname 	= str;
		const char 				* srvc		= "telnet";
		char					* ptr;
		char					* query		= "";
		int						  f;

		trace("%s(%s)",__FUNCTION__,str);
		h->options = LIB3270_OPTION_DEFAULTS;

		for(f=0;f < sizeof(sch)/sizeof(sch[0]);f++)
		{
			size_t sz = strlen(sch[f].text);
			if(!strncasecmp(hostname,sch[f].text,sz))
			{
				h->options	 = sch[f].opt;
				srvc		 = sch[f].srvc;
				hostname	+= sz;
				break;
			}
		}

		if(!*hostname)
			return h->host.current;

		ptr = strchr(hostname,':');
		if(ptr)
		{
			*(ptr++) = 0;
			srvc  = ptr;
			query = strchr(ptr,'?');

			trace("QUERY=[%s]",query);

			if(query)
				*(query++) = 0;
			else
				query = "";
		}

		trace("SRVC=[%s]",srvc);

		Replace(h->host.current,strdup(hostname));
		Replace(h->host.srvc,strdup(srvc));

		// Verifica parâmetros
		if(query && *query)
		{
			char *str 		= strdup(query);
			char *ptr;

#ifdef HAVE_STRTOK_R
			char *saveptr	= NULL;
			for(ptr = strtok_r(str,"&",&saveptr);ptr;ptr=strtok_r(NULL,"&",&saveptr))
#else
			for(ptr = strtok(str,"&");ptr;ptr=strtok(NULL,"&"))
#endif
			{
				char *var = ptr;
				char *val = strchr(ptr,'=');
				if(val)
				{
					*(val++) = 0;

					if(!(strcasecmp(var,"lu") && strcasecmp(var,"luname")))
					{
						strncpy(h->luname,val,LIB3270_LUNAME_LENGTH);
					}
					else
					{
						lib3270_write_log(h,"","Ignoring invalid URL attribute \"%s\"",var);
					}


				}

			}

			free(str);
		}

		// Notifica atualização
		update_host(h);

		free(str);
	}

	return h->host.current;
}

LIB3270_EXPORT const char * lib3270_get_hostname(H3270 *h)
{
    CHECK_SESSION_HANDLE(h);

	if(h->host.current)
		return h->host.current;

	return "";
}

LIB3270_EXPORT void lib3270_set_hostname(H3270 *h, const char *hostname)
{
    CHECK_SESSION_HANDLE(h);
	Replace(h->host.current,strdup(hostname));
	update_host(h);
}

LIB3270_EXPORT const char * lib3270_get_srvcname(H3270 *h)
{
    CHECK_SESSION_HANDLE(h);
    if(h->host.srvc)
		return h->host.srvc;
	return "telnet";
}

LIB3270_EXPORT void lib3270_set_srvcname(H3270 *h, const char *srvc)
{
    CHECK_SESSION_HANDLE(h);
	Replace(h->host.srvc,strdup(srvc));
	update_host(h);
}

LIB3270_EXPORT const char * lib3270_get_host(H3270 *h)
{
    CHECK_SESSION_HANDLE(h);
	return h->host.full;
}

LIB3270_EXPORT const char * lib3270_get_luname(H3270 *h)
{
    CHECK_SESSION_HANDLE(h);
	return h->connected_lu;
}

LIB3270_EXPORT int lib3270_has_active_script(H3270 *h)
{
    CHECK_SESSION_HANDLE(h);
	return (h->oia_flag[LIB3270_FLAG_SCRIPT] != 0);
}

LIB3270_EXPORT int lib3270_get_typeahead(H3270 *h)
{
    CHECK_SESSION_HANDLE(h);
	return (h->oia_flag[LIB3270_FLAG_TYPEAHEAD] != 0);
}

LIB3270_EXPORT int lib3270_get_undera(H3270 *h)
{
    CHECK_SESSION_HANDLE(h);
	return (h->oia_flag[LIB3270_FLAG_UNDERA] != 0);
}

LIB3270_EXPORT int lib3270_get_oia_box_solid(H3270 *h)
{
    CHECK_SESSION_HANDLE(h);
	return (h->oia_flag[LIB3270_FLAG_BOXSOLID] != 0);
}

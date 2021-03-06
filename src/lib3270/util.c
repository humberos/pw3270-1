/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob
 * o nome G3270.
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
 * Este programa está nomeado como util.c e possui - linhas de código.
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
 *	util.c
 *		Utility functions for x3270/c3270/s3270/tcl3270
 */

#define _GNU_SOURCE

#if defined(_WIN32)
	#include <winsock2.h>
	#include <windows.h>
#endif // _WIN32

#include "private.h"

#if defined(_WIN32)

	#include "winversc.h"
	#include <ws2tcpip.h>
	#include <stdio.h>
	#include <errno.h>
	#include "w3miscc.h"

#else
	#include <pwd.h>
#endif // _WIN32

#ifdef HAVE_MALLOC_H
	#include <malloc.h>
#endif

#ifndef ANDROID
	#include <stdlib.h>
#endif // !ANDROID

#ifdef HAVE_ICONV
	#include <iconv.h>
#endif // HAVE_ICONV

#include <stdarg.h>
#include "resources.h"

#include "utilc.h"
#include "popupsc.h"
#include "api.h"

#include <lib3270/session.h>
#include <lib3270/selection.h>

#define my_isspace(c)	isspace((unsigned char)c)

#if defined(_WIN32)

int is_nt = 1;
int has_ipv6 = 1;

int get_version_info(void)
{
	OSVERSIONINFO info;

	// Figure out what version of Windows this is.
	memset(&info, '\0', sizeof(info));
	info.dwOSVersionInfoSize = sizeof(info);
	if(GetVersionEx(&info) == 0)
	{
		lib3270_write_log(NULL,"lib3270","%s","Can't get Windows version");
		return -1;
	}

	// Yes, people still run Win98.
	if (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		is_nt = 0;

	// Win2K and earlier is IPv4-only.  WinXP and later can have IPv6.
	if (!is_nt || info.dwMajorVersion < 5 || (info.dwMajorVersion == 5 && info.dwMinorVersion < 1))
	{
		has_ipv6 = 0;
	}

	return 0;
}

// Convert a network address to a string.
#ifndef HAVE_INET_NTOP
const char * inet_ntop(int af, const void *src, char *dst, socklen_t cnt)
{
    	union {
	    	struct sockaddr sa;
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
	} sa;
	DWORD ssz;
	DWORD sz = cnt;

	memset(&sa, '\0', sizeof(sa));

	switch (af) {
	case AF_INET:
	    	sa.sin = *(struct sockaddr_in *)src;	// struct copy
		ssz = sizeof(struct sockaddr_in);
		break;
	case AF_INET6:
	    	sa.sin6 = *(struct sockaddr_in6 *)src;	// struct copy
		ssz = sizeof(struct sockaddr_in6);
		break;
	default:
	    	if (cnt > 0)
			dst[0] = '\0';
		return NULL;
	}

	sa.sa.sa_family = af;

	if (WSAAddressToString(&sa.sa, ssz, NULL, dst, &sz) != 0) {
	    	if (cnt > 0)
			dst[0] = '\0';
		return NULL;
	}

	return dst;
}
#endif // HAVE_INET_NTOP

// Decode a Win32 error number.
LIB3270_EXPORT const char * lib3270_win32_strerror(int e)
{
	static char buffer[4096];

	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,e,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),buffer,sizeof(buffer),NULL) == 0)
	{
	    snprintf(buffer, 4095, _( "Windows error %d" ), e);
		return buffer;
	}

#ifdef HAVE_ICONV
	{
		// Convert from windows codepage to UTF-8 pw3270´s default charset
		iconv_t hConv = iconv_open("UTF-8",lib3270_win32_local_charset());

		trace("[%s]",buffer);

		if(hConv == (iconv_t) -1)
		{
			lib3270_write_log(NULL,"iconv","%s: Error creating charset conversion",__FUNCTION__);
		}
		else
		{
			size_t				  in 		= strlen(buffer);
			size_t				  out 		= (in << 1);
			char				* ptr;
			char				* outBuffer = (char *) malloc(out);
			ICONV_CONST char	* inBuffer	= (ICONV_CONST char	*) buffer;

			memset(ptr=outBuffer,0,out);

			iconv(hConv,NULL,NULL,NULL,NULL);	// Reset state

			if(iconv(hConv,&inBuffer,&in,&ptr,&out) != ((size_t) -1))
			{
				strncpy(buffer,outBuffer,4095);
			}

			free(outBuffer);

			iconv_close(hConv);
		}

	}
#endif // HAVE_ICONV

	return buffer;
}

LIB3270_EXPORT const char * lib3270_win32_local_charset(void)
{
	// Reference:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd318070(v=vs.85).aspx

	#warning TODO: Use GetACP() to identify the correct code page

	trace("Windows CHARSET is %u",GetACP());

	return "CP1252";
}


#endif // _WIN32

/*
 * Cheesy internal version of sprintf that allocates its own memory.
 */
char * lib3270_vsprintf(const char *fmt, va_list args)
{
	char *r = NULL;

#if defined(HAVE_VASPRINTF)

	if(vasprintf(&r, fmt, args) < 0 || !r)
		Error(NULL,"Out of memory in %s",__FUNCTION__);

#else

	char buf[16384];
	int nc;

	nc = vsnprintf(buf, sizeof(buf), fmt, args);
	if(nc < 0)
	{
		Error(NULL,"Out of memory in %s",__FUNCTION__);
	}
	else if (nc < sizeof(buf))
	{
		r = lib3270_malloc(nc + 1);
		strcpy(r, buf);

	}
	else
	{
		r = lib3270_malloc(nc + 1);
		if(vsnprintf(r, nc, fmt, args) < 0)
			Error(NULL,"Out of memory in %s",__FUNCTION__);

	}

#endif

	return r;
}

LIB3270_EXPORT char * lib3270_strdup_printf(const char *fmt, ...)
{
	va_list args;
	char *r;

	va_start(args, fmt);
	r = lib3270_vsprintf(fmt, args);
	va_end(args);
	return r;
}

/*
 * Common helper functions to insert strings, through a template, into a new
 * buffer.
 * 'format' is assumed to be a printf format string with '%s's in it.
 */
char * xs_buffer(const char *fmt, ...)
{
	va_list args;
	char *r;

	va_start(args, fmt);
	r = lib3270_vsprintf(fmt, args);
	va_end(args);
	return r;
}

/* Common uses of xs_buffer. */
void
xs_warning(const char *fmt, ...)
{
	va_list args;
	char *r;

	va_start(args, fmt);
	r = lib3270_vsprintf(fmt, args);
	va_end(args);
	Warning(NULL,r);
	lib3270_free(r);
}

void
xs_error(const char *fmt, ...)
{
	va_list args;
	char *r;

	va_start(args, fmt);
	r = lib3270_vsprintf(fmt, args);
	va_end(args);
	Error(NULL,r);
	lib3270_free(r);
}


/*
 * Definition resource splitter, for resources of the repeating form:
 *	left: right\n
 *
 * Can be called iteratively to parse a list.
 * Returns 1 for success, 0 for EOF, -1 for error.
 *
 * Note: Modifies the input string.
 */
int
split_dresource(char **st, char **left, char **right)
{
	char *s = *st;
	char *t;
	Boolean quote;

	/* Skip leading white space. */
	while (my_isspace(*s))
		s++;

	/* If nothing left, EOF. */
	if (!*s)
		return 0;

	/* There must be a left-hand side. */
	if (*s == ':')
		return -1;

	/* Scan until an unquoted colon is found. */
	*left = s;
	for (; *s && *s != ':' && *s != '\n'; s++)
		if (*s == '\\' && *(s+1) == ':')
			s++;
	if (*s != ':')
		return -1;

	/* Stip white space before the colon. */
	for (t = s-1; my_isspace(*t); t--)
		*t = '\0';

	/* Terminate the left-hand side. */
	*(s++) = '\0';

	/* Skip white space after the colon. */
	while (*s != '\n' && my_isspace(*s))
		s++;

	/* There must be a right-hand side. */
	if (!*s || *s == '\n')
		return -1;

	/* Scan until an unquoted newline is found. */
	*right = s;
	quote = False;
	for (; *s; s++) {
		if (*s == '\\' && *(s+1) == '"')
			s++;
		else if (*s == '"')
			quote = !quote;
		else if (!quote && *s == '\n')
			break;
	}

	/* Strip white space before the newline. */
	if (*s) {
		t = s;
		*st = s+1;
	} else {
		t = s-1;
		*st = s;
	}
	while (my_isspace(*t))
		*t-- = '\0';

	/* Done. */
	return 1;
}

/*
 * Split a DBCS resource into its parts.
 * Returns the number of parts found:
 *	-1 error (empty sub-field)
 *	 0 nothing found
 *	 1 one and just one thing found
 *	 2 two things found
 *	 3 more than two things found
 */
int
split_dbcs_resource(const char *value, char sep, char **part1, char **part2)
{
	int n_parts = 0;
	const char *s = value;
	const char *f_start = CN;	/* start of sub-field */
	const char *f_end = CN;		/* end of sub-field */
	char c;
	char **rp;

	*part1 = CN;
	*part2 = CN;

	for( ; ; ) {
		c = *s;
		if (c == sep || c == '\0') {
			if (f_start == CN)
				return -1;
			if (f_end == CN)
				f_end = s;
			if (f_end == f_start) {
				if (c == sep) {
					if (*part1) {
						lib3270_free(*part1);
						*part1 = NULL;
					}
					if (*part2) {
						lib3270_free(*part2);
						*part2 = NULL;
					}
					return -1;
				} else
					return n_parts;
			}
			switch (n_parts) {
			case 0:
				rp = part1;
				break;
			case 1:
				rp = part2;
				break;
			default:
				return 3;
			}
			*rp = lib3270_malloc(f_end - f_start + 1);
			strncpy(*rp, f_start, f_end - f_start);
			(*rp)[f_end - f_start] = '\0';
			f_end = CN;
			f_start = CN;
			n_parts++;
			if (c == '\0')
				return n_parts;
		} else if (isspace(c)) {
			if (f_end == CN)
				f_end = s;
		} else {
			if (f_start == CN)
				f_start = s;
			f_end = CN;
		}
		s++;
	}
}

#if defined(X3270_DISPLAY) /*[*/
/*
 * List resource splitter, for lists of elements speparated by newlines.
 *
 * Can be called iteratively.
 * Returns 1 for success, 0 for EOF, -1 for error.
 */
int
split_lresource(char **st, char **value)
{
	char *s = *st;
	char *t;
	Boolean quote;

	/* Skip leading white space. */
	while (my_isspace(*s))
		s++;

	/* If nothing left, EOF. */
	if (!*s)
		return 0;

	/* Save starting point. */
	*value = s;

	/* Scan until an unquoted newline is found. */
	quote = False;
	for (; *s; s++) {
		if (*s == '\\' && *(s+1) == '"')
			s++;
		else if (*s == '"')
			quote = !quote;
		else if (!quote && *s == '\n')
			break;
	}

	/* Strip white space before the newline. */
	if (*s) {
		t = s;
		*st = s+1;
	} else {
		t = s-1;
		*st = s;
	}
	while (my_isspace(*t))
		*t-- = '\0';

	/* Done. */
	return 1;
}
#endif /*]*/


/*
#if !defined(LIB3270)

const char *
get_message(const char *key)
{
	static char namebuf[128];
	char *r;

	(void) sprintf(namebuf, "%s.%s", ResMessage, key);
	if ((r = get_resource(namebuf)) != CN)
		return r;
	else {
		(void) sprintf(namebuf, "[missing \"%s\" message]", key);
		return namebuf;
	}
}

#endif
*/

// #define ex_getenv getenv

/* Variable and tilde substitution functions. */ /*
static char *
var_subst(const char *s)
{
	enum { VS_BASE, VS_QUOTE, VS_DOLLAR, VS_BRACE, VS_VN, VS_VNB, VS_EOF }
	    state = VS_BASE;
	char c;
	int o_len = strlen(s) + 1;
	char *ob;
	char *o;
	const char *vn_start = CN;

	if (strchr(s, '$') == CN)
		return NewString(s);

	o_len = strlen(s) + 1;
	ob = lib3270_malloc(o_len);
	o = ob;
#	define LBR	'{'
#	define RBR	'}'

	while (state != VS_EOF) {
		c = *s;
		switch (state) {
		    case VS_BASE:
			if (c == '\\')
			    state = VS_QUOTE;
			else if (c == '$')
			    state = VS_DOLLAR;
			else
			    *o++ = c;
			break;
		    case VS_QUOTE:
			if (c == '$') {
				*o++ = c;
				o_len--;
			} else {
				*o++ = '\\';
				*o++ = c;
			}
			state = VS_BASE;
			break;
		    case VS_DOLLAR:
			if (c == LBR)
				state = VS_BRACE;
			else if (isalpha(c) || c == '_') {
				vn_start = s;
				state = VS_VN;
			} else {
				*o++ = '$';
				*o++ = c;
				state = VS_BASE;
			}
			break;
		    case VS_BRACE:
			if (isalpha(c) || c == '_') {
				vn_start = s;
				state = VS_VNB;
			} else {
				*o++ = '$';
				*o++ = LBR;
				*o++ = c;
				state = VS_BASE;
			}
			break;
		    case VS_VN:
		    case VS_VNB:
			if (!(isalnum(c) || c == '_')) {
				int vn_len;
				char *vn;
				char *vv;

				vn_len = s - vn_start;
				if (state == VS_VNB && c != RBR) {
					*o++ = '$';
					*o++ = LBR;
					(void) strncpy(o, vn_start, vn_len);
					o += vn_len;
					state = VS_BASE;
					continue;	// rescan
				}
				vn = lib3270_malloc(vn_len + 1);
				(void) strncpy(vn, vn_start, vn_len);
				vn[vn_len] = '\0';

#ifndef ANDROID
				if((vv = getenv(vn)))
				{
					*o = '\0';
					o_len = o_len
					    - 1			// '$'
					    - (state == VS_VNB)	//
					    - vn_len		// name
					    - (state == VS_VNB)	// }
					    + strlen(vv);
					ob = Realloc(ob, o_len);
					o = strchr(ob, '\0');
					(void) strcpy(o, vv);
					o += strlen(vv);
				}
#endif // !ANDROID

				lib3270_free(vn);
				if (state == VS_VNB) {
					state = VS_BASE;
					break;
				} else {
					// Rescan this character
					state = VS_BASE;
					continue;
				}
			}
			break;
		    case VS_EOF:
			break;
		}
		s++;
		if (c == '\0')
			state = VS_EOF;
	}
	return ob;
}
*/

#if !defined(_WIN32)
/*
 * Do tilde (home directory) substitution on a string.  Returns a malloc'd
 * result.
 */
 /*
static char *
tilde_subst(const char *s)
{
	char *slash;
	const char *name;
	const char *rest;
	struct passwd *p;
	char *r;
	char *mname = CN;

	// Does it start with a "~"?
	if (*s != '~')
		return NewString(s);

	// Terminate with "/".
	slash = strchr(s, '/');
	if (slash) {
		int len = slash - s;

		mname = lib3270_malloc(len + 1);
		(void) strncpy(mname, s, len);
		mname[len] = '\0';
		name = mname;
		rest = slash;
	} else {
		name = s;
		rest = strchr(name, '\0');
	}

	// Look it up.
	if (!strcmp(name, "~"))	// this user
		p = getpwuid(getuid());
	else			// somebody else
		p = getpwnam(name + 1);

	// Free any temporary copy.
	lib3270_free(mname);

	// Substitute and return.
	if (p == (struct passwd *)NULL)
		r = NewString(s);
	else {
		r = lib3270_malloc(strlen(p->pw_dir) + strlen(rest) + 1);
		(void) strcpy(r, p->pw_dir);
		(void) strcat(r, rest);
	}
	return r;
} */
#endif
/*
char *
do_subst(const char *s, Boolean do_vars, Boolean do_tilde)
{
	if (!do_vars && !do_tilde)
		return NewString(s);

	if (do_vars) {
		char *t;

		t = var_subst(s);
#if !defined(_WIN32)
		if (do_tilde) {
			char *u;

			u = tilde_subst(t);
			lib3270_free(t);
			return u;
		}
#endif
		return t;
	}

#if !defined(_WIN32)
	return tilde_subst(s);
#else
	return NewString(s);
#endif
}

*/

/*
 * ctl_see
 *	Expands a character in the manner of "cat -v".
 */
char *
ctl_see(int c)
{
	static char	buf[64];
	char	*p = buf;

	c &= 0xff;
	if ((c & 0x80) && (c <= 0xa0)) {
		*p++ = 'M';
		*p++ = '-';
		c &= 0x7f;
	}
	if (c >= ' ' && c != 0x7f) {
		*p++ = c;
	} else {
		*p++ = '^';
		if (c == 0x7f) {
			*p++ = '?';
		} else {
			*p++ = c + '@';
		}
	}
	*p = '\0';
	return buf;
}

/*
 * Whitespace stripper.
 */
char *
strip_whitespace(const char *s)
{
	char *t = NewString(s);

	while (*t && my_isspace(*t))
		t++;
	if (*t) {
		char *u = t + strlen(t) - 1;

		while (my_isspace(*u)) {
			*u-- = '\0';
		}
	}
	return t;
}

/*
 * Hierarchy (a>b>c) splitter.
 */
Boolean
split_hier(char *label, char **base, char ***parents)
{
	int n_parents = 0;
	char *gt;
	char *lp;

	label = NewString(label);
	for (lp = label; (gt = strchr(lp, '>')) != CN; lp = gt + 1) {
		if (gt == lp)
			return False;
		n_parents++;
	}
	if (!*lp)
		return False;

	if (n_parents) {
		*parents = (char **)Calloc(n_parents + 1, sizeof(char *));
		for (n_parents = 0, lp = label;
		     (gt = strchr(lp, '>')) != CN;
		     lp = gt + 1) {
			(*parents)[n_parents++] = lp;
			*gt = '\0';
		}
		*base = lp;
	} else {
		(*parents) = NULL;
		(*base) = label;
	}
	return True;
}

/*
 * Incremental, reallocing version of snprintf.
 */
#define RPF_BLKSIZE	4096
#define SP_TMP_LEN	16384

/* Initialize an RPF structure. */
void
rpf_init(rpf_t *r)
{
	r->buf = NULL;
	r->alloc_len = 0;
	r->cur_len = 0;
}

/* Reset an initialized RPF structure (re-use with length 0). */
void
rpf_reset(rpf_t *r)
{
	r->cur_len = 0;
}

/* Append a string to a dynamically-allocated buffer. */
void
rpf(rpf_t *r, char *fmt, ...)
{
	va_list a;
	Boolean need_realloc = False;
	int ns;
	char tbuf[SP_TMP_LEN];

	/* Figure out how much space would be needed. */
	va_start(a, fmt);
	ns = vsprintf(tbuf, fmt, a); /* XXX: dangerous, but so is vsnprintf */
	va_end(a);
	if (ns >= SP_TMP_LEN)
	    Error(NULL,"rpf overrun");

	/* Make sure we have that. */
	while (r->alloc_len - r->cur_len < ns + 1) {
		r->alloc_len += RPF_BLKSIZE;
		need_realloc = True;
	}
	if (need_realloc) {
		r->buf = Realloc(r->buf, r->alloc_len);
	}

	/* Scribble onto the end of that. */
	(void) strcpy(r->buf + r->cur_len, tbuf);
	r->cur_len += ns;
}

/* Free resources associated with an RPF. */
void
rpf_free(rpf_t *r)
{
	lib3270_free(r->buf);
	r->buf = NULL;
	r->alloc_len = 0;
	r->cur_len = 0;
}

LIB3270_EXPORT void * lib3270_free(void *p)
{
	if(p)
		free(p);
	return NULL;
}

LIB3270_EXPORT void * lib3270_realloc(void *p, int len)
{
	p = realloc(p, len);
	if(!p)
		Error(NULL,"Out of memory in %s",__FUNCTION__);
	return p;
}

LIB3270_EXPORT void * lib3270_calloc(int elsize, int nelem, void *ptr)
{
	size_t sz = nelem * elsize;

	if(ptr)
		ptr = realloc(ptr,sz);
	else
		ptr = malloc(sz);

	if(ptr)
		memset(ptr,0,sz);
	else
		Error(NULL,"Out of memory in %s",__FUNCTION__);

	return ptr;
}


LIB3270_EXPORT void * lib3270_malloc(int len)
{
	char *r;

	r = malloc(len);
	if (r == (char *)NULL)
	{
		Error(NULL,"Out of memory in %s",__FUNCTION__);
		return 0;
	}

	memset(r,0,len);
	return r;
}

LIB3270_EXPORT void * lib3270_strdup(const char *str)
{
	char *r;

	r = strdup(str);
	if (r == (char *)NULL)
	{
		Error(NULL,"Out of memory in %s",__FUNCTION__);
		return 0;
	}

	return r;
}

LIB3270_EXPORT const char * lib3270_get_version(void)
{
	return build_rpq_version;
}

LIB3270_EXPORT const char * lib3270_get_revision(void)
{
	return build_rpq_revision;
}

void lib3270_popup_an_errno(H3270 *hSession, int errn, const char *fmt, ...)
{
	va_list	  args;
	char	* text;

	va_start(args, fmt);
	text = lib3270_vsprintf(fmt, args);
	va_end(args);

	lib3270_write_log(hSession, "3270", "Error Popup:\n%s\nrc=%d (%s)",text,errn,strerror(errn));

	lib3270_popup_dialog(hSession, LIB3270_NOTIFY_ERROR, _( "Error" ), text, "%s (rc=%d)", errn, strerror(errn));

	lib3270_free(text);

}

#if defined(_WIN32)

#define SECS_BETWEEN_EPOCHS	11644473600ULL
#define SECS_TO_100NS		10000000ULL /* 10^7 */

int gettimeofday(struct timeval *tv, void *ignored)
{
	FILETIME t;
	ULARGE_INTEGER u;

	GetSystemTimeAsFileTime(&t);
	memcpy(&u, &t, sizeof(ULARGE_INTEGER));

	/* Isolate seconds and move epochs. */
	tv->tv_sec = (DWORD)((u.QuadPart / SECS_TO_100NS) - SECS_BETWEEN_EPOCHS);
	tv->tv_usec = (u.QuadPart % SECS_TO_100NS) / 10ULL;
	return 0;
}

#endif

 LIB3270_EXPORT int lib3270_print(H3270 *h)
 {
	CHECK_SESSION_HANDLE(h);
	trace("%s(%p)",__FUNCTION__,h);
	return h->cbk.print(h);
 }


 LIB3270_EXPORT LIB3270_POINTER lib3270_get_pointer(H3270 *hSession, int baddr)
 {
	static const struct _ptr {
		unsigned short	id;
		LIB3270_POINTER	value;
	} ptr[] = {
		{ 0x80,	LIB3270_POINTER_MOVE_SELECTION			},
		{ 0x82,	LIB3270_POINTER_SELECTION_TOP			},
		{ 0x86,	LIB3270_POINTER_SELECTION_TOP_RIGHT		},
		{ 0x84,	LIB3270_POINTER_SELECTION_RIGHT			},
		{ 0x81,	LIB3270_POINTER_SELECTION_LEFT			},
		{ 0x89,	LIB3270_POINTER_SELECTION_BOTTOM_LEFT	},
		{ 0x88,	LIB3270_POINTER_SELECTION_BOTTOM		},
		{ 0x8c,	LIB3270_POINTER_SELECTION_BOTTOM_RIGHT	},
		{ 0x83,	LIB3270_POINTER_SELECTION_TOP_LEFT		}
	};

	int f;
	unsigned short id = lib3270_get_selection_flags(hSession,baddr) & 0x8f;

	if(!lib3270_connected(hSession) || baddr < 0)
		return LIB3270_POINTER_LOCKED;

	for(f = 0; f < (sizeof(ptr)/sizeof(ptr[0]));f++)
	{
		if(ptr[f].id == id)
		{
			return ptr[f].value;
		}
	}

	return hSession->pointer;

 }

 LIB3270_EXPORT int lib3270_getpeername(H3270 *hSession, struct sockaddr *addr, socklen_t *addrlen)
 {
	CHECK_SESSION_HANDLE(hSession);

 	memset(addr,0,*addrlen);

 	if(hSession->sock < 0) {
		errno = ENOTCONN;
		return -1;
 	}

	return getpeername(hSession->sock, addr, addrlen);

 }

 LIB3270_EXPORT int lib3270_getsockname(H3270 *hSession, struct sockaddr *addr, socklen_t *addrlen)
 {
	CHECK_SESSION_HANDLE(hSession);

 	memset(addr,0,*addrlen);

 	if(hSession->sock < 0) {
		errno = ENOTCONN;
		return -1;
 	}

	return getsockname(hSession->sock, addr, addrlen);
 }

/*
 * "Software PW3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
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
 * Este programa está nomeado como api.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 */


#ifndef LIB3270_API_INCLUDED

#ifdef WIN32
	#include <winsock2.h>
	#include <windows.h>
#endif // WIN32

#ifdef __cplusplus
	extern "C" {
#endif

		#define LIB3270_API_INCLUDED "4.2"

		#include <errno.h>

		#if defined(_WIN32)
			#include <windows.h>

			#if defined (HAVE_GNUC_VISIBILITY)
				#define LOCAL_EXTERN	__attribute__((visibility("hidden"))) extern
			#else
				#define LOCAL_EXTERN extern
			#endif

		#else
			#include <stdarg.h>

			// http://gcc.gnu.org/wiki/Visibility
			#if defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
				#define LOCAL_EXTERN __hidden extern
			#elif defined (HAVE_GNUC_VISIBILITY)
				#define LOCAL_EXTERN	__attribute__((visibility("hidden"))) extern
			#else
				#define LOCAL_EXTERN extern
			#endif


		#endif

		#ifndef HCONSOLE
			#define HCONSOLE void *
		#endif

		#ifndef ETIMEDOUT
			#define ETIMEDOUT -1238
		#endif

		#ifndef ECANCELED
			#ifdef EINTR
				#define ECANCELED EINTR
			#else
				#define ECANCELED -1125
			#endif
		#endif

		#ifndef ENOTCONN
			#define ENOTCONN -1107
		#endif

		#ifndef CN
			#define CN ((char *) NULL)
		#endif

		#include <lib3270/log.h>

//		#define WriteLog(module,fmt, ...) 		lib3270_write_log(NULL,module,fmt,__VA_ARGS__)
//		#define WriteRCLog(module,rc,fmt, ...)	lib3270_write_rc(NULL,module,fmt,__VA_ARGS__)

/*
		#ifdef LIB3270_MODULE_NAME
			#define Log(fmt, ...)		lib3270_write_log(NULL,LIB3270_MODULE_NAME,fmt,__VA_ARGS__)
		#else
			#define Log(fmt, ...)		lib3270_write_log(NULL,"MSG",fmt,__VA_ARGS__)
		#endif
*/

		/** 3270 connection handle */
//		#define LUNAME_SIZE				16
//		#define FULL_MODEL_NAME_SIZE	13

//		#define ST_RESOLVING			LIB3270_STATE_RESOLVING
//		#define ST_HALF_CONNECT			LIB3270_STATE_HALF_CONNECT
//		#define ST_CONNECT				LIB3270_STATE_CONNECT
//		#define ST_3270_MODE			LIB3270_STATE_3270_MODE
//		#define ST_LINE_MODE			LIB3270_STATE_LINE_MODE
//		#define ST_REMODEL				LIB3270_STATE_REMODEL
//		#define ST_PRINTER				LIB3270_STATE_PRINTER
//		#define ST_EXITING				LIB3270_STATE_EXITING
//		#define ST_CHARSET				LIB3270_STATE_CHARSET
//		#define N_ST					LIB3270_STATE_USER
//		#define LIB3270_STATE_CHANGE	LIB3270_STATE

		/** connection state */
//		#define cstate LIB3270_CSTATE
//		#define NOT_CONNECTED	LIB3270_NOT_CONNECTED
//		#define RESOLVING	LIB3270_RESOLVING
//		#define PENDING	LIB3270_PENDING
//		#define CONNECTED_INITIAL	LIB3270_CONNECTED_INITIAL
//		#define CONNECTED_ANSI	LIB3270_CONNECTED_ANSI
//		#define CONNECTED_3270	LIB3270_CONNECTED_3270
//		#define CONNECTED_INITIAL_E	LIB3270_CONNECTED_INITIAL_E
//		#define CONNECTED_NVT	LIB3270_CONNECTED_NVT
//		#define CONNECTED_SSCP	LIB3270_CONNECTED_SSCP
//		#define CONNECTED_TN3270E	LIB3270_CONNECTED_TN3270E

//		#define LIB3270_STATUS					LIB3270_MESSAGE
//		#define LIB3270_STATUS_BLANK			LIB3270_MESSAGE_NONE
//		#define LIB3270_STATUS_SYSWAIT			LIB3270_MESSAGE_SYSWAIT
//		#define LIB3270_STATUS_TWAIT			LIB3270_MESSAGE_TWAIT
//		#define LIB3270_STATUS_CONNECTED		LIB3270_MESSAGE_CONNECTED
//		#define LIB3270_STATUS_DISCONNECTED		LIB3270_MESSAGE_DISCONNECTED
//		#define LIB3270_STATUS_AWAITING_FIRST	LIB3270_MESSAGE_AWAITING_FIRST
//		#define LIB3270_MESSAGE_MINUS			LIB3270_MESSAGE_MINUS
//		#define LIB3270_STATUS_PROTECTED		LIB3270_MESSAGE_PROTECTED
//		#define LIB3270_STATUS_NUMERIC			LIB3270_MESSAGE_NUMERIC
//		#define LIB3270_STATUS_OVERFLOW			LIB3270_MESSAGE_OVERFLOW
//		#define LIB3270_STATUS_INHIBIT			LIB3270_MESSAGE_INHIBIT
//		#define LIB3270_STATUS_KYBDLOCK			LIB3270_MESSAGE_KYBDLOCK
//		#define LIB3270_STATUS_X				LIB3270_MESSAGE_X
//		#define LIB3270_MESSAGE_RESOLVING		LIB3270_MESSAGE_RESOLVING
//		#define LIB3270_STATUS_CONNECTING		LIB3270_MESSAGE_CONNECTING
//		#define LIB3270_STATUS_USER				LIB3270_MESSAGE_USER

		#define OIA_FLAG_BOXSOLID	LIB3270_FLAG_BOXSOLID
		#define OIA_FLAG_UNDERA		LIB3270_FLAG_UNDERA
		#define OIA_FLAG_TYPEAHEAD	LIB3270_FLAG_TYPEAHEAD
		#define OIA_FLAG_USER		LIB3270_FLAG_COUNT
		#define OIA_FLAG			LIB3270_FLAG

		struct lib3270_state_callback;

		#include <lib3270/session.h>

		struct lib3270_state_callback
		{
			struct lib3270_state_callback	* next;			/**< Next callback in chain */
			void							* data;			/**< User data */
			void (*func)(H3270 *, int, void *);		/**< Function to call */
		};


		/** Type of dialog boxes */
		#include <lib3270/popup.h>

		#define PW3270_DIALOG_INFO		LIB3270_NOTIFY_INFO
		#define PW3270_DIALOG_CRITICAL	LIB3270_NOTIFY_CRITICAL
		#define PW3270_DIALOG			LIB3270_NOTIFY

		#define GR_BLINK		0x01
		#define GR_REVERSE		0x02
		#define GR_UNDERLINE	0x04
		#define GR_INTENSIFY	0x08

		#define CS_MASK			0x03	/**< mask for specific character sets */
		#define CS_BASE			0x00	/**< base character set (X'00') */
		#define CS_APL			0x01	/**< APL character set (X'01' or GE) */
		#define CS_LINEDRAW		0x02	/**< DEC line-drawing character set (ANSI) */
		#define CS_DBCS			0x03	/**< DBCS character set (X'F8') */
		#define CS_GE			0x04	/**< cs flag for Graphic Escape */

		/**
		 * Return a "malloced" copy of the device buffer, set number of elements
		 */
//		LOCAL_EXTERN struct ea * copy_device_buffer(int *el);

		/**
		 * Set the contents of the device buffer for debugging purposes
		 */
//		LOCAL_EXTERN int  set_device_buffer(struct ea *src, int el);

		/* File transfer */

		#define FT_RECORD_FORMAT_FIXED			LIB3270_FT_RECORD_FORMAT_FIXED
		#define FT_RECORD_FORMAT_VARIABLE		LIB3270_FT_RECORD_FORMAT_VARIABLE
		#define FT_RECORD_FORMAT_UNDEFINED		LIB3270_FT_RECORD_FORMAT_UNDEFINED
		#define FT_RECORD_FORMAT_MASK 			LIB3270_FT_RECORD_FORMAT_MASK

		#define FT_ALLOCATION_UNITS_TRACKS		LIB3270_FT_ALLOCATION_UNITS_TRACKS
		#define FT_ALLOCATION_UNITS_CYLINDERS	LIB3270_FT_ALLOCATION_UNITS_CYLINDERS
		#define FT_ALLOCATION_UNITS_AVBLOCK		LIB3270_FT_ALLOCATION_UNITS_AVBLOCK
		#define FT_ALLOCATION_UNITS_MASK		LIB3270_FT_ALLOCATION_UNITS_MASK

		#define FT_NONE							LIB3270_FT_STATE_NONE
		#define FT_AWAIT_ACK					LIB3270_FT_STATE_AWAIT_ACK
		#define FT_RUNNING						LIB3270_FT_STATE_RUNNING
		#define FT_ABORT_WAIT					LIB3270_FT_STATE_ABORT_WAIT
		#define FT_ABORT_SENT					LIB3270_FT_STATE_ABORT_SENT

//		LOCAL_EXTERN int 				BeginFileTransfer(unsigned short flags, const char *local, const char *remote, int lrecl, int blksize, int primspace, int secspace, int dft);
		LOCAL_EXTERN int 				CancelFileTransfer(int force);
//		LOCAL_EXTERN enum ft_state	GetFileTransferState(void);

/*
		struct filetransfer_callbacks
		{
			unsigned short sz;

			void (*begin)(unsigned short flags, const char *local, const char *remote);
			void (*complete)(const char *errmsg,unsigned long length,double kbytes_sec,const char *mode);
			void (*update)(unsigned long length,unsigned long total,double kbytes_sec);
			void (*running)(int is_cut);
			void (*aborting)(void);

		};

		LOCAL_EXTERN int RegisterFTCallbacks(const struct filetransfer_callbacks *cbk);
*/

		#define PCONNECTED		lib3270_pconnected(hSession)
		#define HALF_CONNECTED	lib3270_half_connected(hSession)
		#define CONNECTED		lib3270_connected(hSession)

		#define IN_NEITHER		lib3270_in_neither(hSession)
		#define IN_ANSI			lib3270_in_ansi(hSession)
		#define IN_3270			lib3270_in_3270(hSession)
		#define IN_SSCP			lib3270_in_sscp(hSession)
		#define IN_TN3270E		lib3270_in_tn3270e(hSession)
		#define IN_E			lib3270_in_e(hSession)

		#ifndef LIB3270

//			LOCAL_EXTERN enum ft_state	QueryFTstate(void);

		#endif


		/* Screen processing */

//		#define CURSOR_MODE_NORMAL		LIB3270_CURSOR_EDITABLE
//		#define CURSOR_MODE_WAITING		LIB3270_CURSOR_WAITING
//		#define CURSOR_MODE_LOCKED		LIB3270_CURSOR_LOCKED

		typedef enum _SCRIPT_STATE
		{
			SCRIPT_STATE_NONE,
			SCRIPT_STATE_RUNNING,
			SCRIPT_STATE_HALTED,

			SCRIPT_STATE_USER

		} SCRIPT_STATE;

		typedef enum _COUNTER_ID
		{
			COUNTER_ID_CTLR_DONE,
			COUNTER_ID_RESET,

			COUNTER_ID_USER
		} COUNTER_ID;

		LOCAL_EXTERN int query_counter(COUNTER_ID id);

		#define	query_screen_change_counter() query_counter(COUNTER_ID_CTLR_DONE)

		#define COLOR_ATTR_NONE			0x0000
		#define COLOR_ATTR_FIELD		LIB3270_ATTR_FIELD
		#define COLOR_ATTR_BLINK		LIB3270_ATTR_BLINK
//		#define COLOR_ATTR_UNDERLINE	LIB3270_ATTR_UNDERLINE
		#define COLOR_ATTR_INTENSIFY	LIB3270_ATTR_INTENSIFY

		#define CHAR_ATTR_UNCONVERTED	LIB3270_ATTR_CG


		/* Set/Get screen contents */
		#define find_field_attribute(s,a) lib3270_field_addr(s,a)
		#define find_field_length(s,a) find_field_length(s,a)

		LOCAL_EXTERN unsigned char get_field_attribute(H3270 *session, int baddr);
//		LOCAL_EXTERN int screen_read(char *dest, int baddr, int count);
		LOCAL_EXTERN void Input_String(const unsigned char *str);
		LOCAL_EXTERN void screen_size(int *rows, int *cols);

//		#define query_secure_connection(h) lib3270_get_ssl_state(h)
		#define lib3270_paste_string(str) lib3270_set_string(NULL,str)
		#define get_3270_terminal_size(h,r,c) lib3270_get_screen_size(h,r,c)

		/* Keyboard */
		LOCAL_EXTERN int			  emulate_input(char *s, int len, int pasting);

		/* Network related calls */
//		LOCAL_EXTERN int 			  Get3270Socket(void);

        /* Misc calls */

		#define query_3270_terminal_status(void) lib3270_get_program_message(NULL)

//		#define set_3270_model(h,m)	lib3270_set_model(h,m)
//		#define get_3270_model(h) lib3270_get_model(h)

		/* Get connection info */
		#define get_connected_lu(h) lib3270_get_luname(h)

		LOCAL_EXTERN SCRIPT_STATE status_script(SCRIPT_STATE state);

		#include <lib3270/actions.h>

		// #define host_connect(n,wait) lib3270_connect(NULL,n,wait)
		// #define host_reconnect(w) lib3270_reconnect(NULL,w)


#ifdef __cplusplus
	}
#endif

#endif // LIB3270_API_INCLUDED

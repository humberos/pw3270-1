/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
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
 * Este programa está nomeado como dialog.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 */

 #include <lib3270/config.h>
 #include "globals.h"
 #include <v3270.h>

 #if defined WIN32
	#include <gdk/gdkwin32.h>

 	struct file {
		OPENFILENAME		 	ofn;
		gboolean				enabled;
		char				  	szName[260];	// buffer for file name
		GtkFileChooserAction	action;
		BOOL					ok;
	};


 #endif // WIN32

 #if defined(HAVE_LIBSSL)
	#include <openssl/ssl.h>
	#include <openssl/err.h>
 #endif

/*--[ Globals ]--------------------------------------------------------------------------------------*/

 static const struct _charset
 {
	const gchar *name;
	const gchar *description;
 } charset[] =
 {
	// http://en.wikipedia.org/wiki/Character_encoding
	{ "UTF-8",		N_( "UTF-8"	)								},
	{ "ISO-8859-1", N_( "Western Europe (ISO 8859-1)" ) 		},
	{ "CP1252",		N_( "Windows Western languages (CP1252)" )	},

	{ NULL, NULL }
 };


/*--[ Implement ]------------------------------------------------------------------------------------*/


 static void charset_changed(GtkComboBox *widget,gchar **encoding)
 {
 	gchar *new_encoding = NULL;

#if GTK_CHECK_VERSION(3,0,0)

	new_encoding = g_strdup(gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget)));

#else

	GValue		value	= { 0, };
	GtkTreeIter iter;

	if(!gtk_combo_box_get_active_iter(widget,&iter))
		return;

	gtk_tree_model_get_value(gtk_combo_box_get_model(widget),&iter,1,&value);
	new_encoding = g_strdup(g_value_get_string(&value));

#endif

	if(!new_encoding)
		return;

	trace("%s: %s->%s",__FUNCTION__,*encoding,new_encoding);
	if(*encoding)
		g_free(*encoding);

	*encoding = new_encoding;
 }

 static void add_option_menus(GtkWidget *widget, GtkAction *action, gchar **encoding)
 {
#if GTK_CHECK_VERSION(3,0,0)
	GtkWidget	*box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,6);
#else
	GtkWidget	*box = gtk_hbox_new(FALSE,6);
#endif // GTK(3,0,0)

 	gchar		*ptr = g_object_get_data(G_OBJECT(action),"charset");

	if(ptr)
	{
		*encoding = g_strdup(ptr);
	}
	else
	{
		// Add charset options
		GtkWidget		* label 	= gtk_label_new_with_mnemonic (_("C_haracter Coding:"));
		const gchar		* scharset	= NULL;
#if GTK_CHECK_VERSION(3,0,0)
		GtkWidget		* menu		= gtk_combo_box_text_new();
#else
		GtkTreeModel	* model		= (GtkTreeModel *) gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);
		GtkWidget		* menu		= gtk_combo_box_new_with_model(model);
		GtkCellRenderer * renderer	= gtk_cell_renderer_text_new();
		GtkTreeIter		  iter;
#endif // GTK(3,0,0)
		gchar			* text;
		int			  f;
		int			  p = 0;

		g_get_charset(&scharset);
		*encoding = g_strdup(scharset);

		text = g_strdup_printf(_("Current (%s)"),scharset);

#if GTK_CHECK_VERSION(3,0,0)

		gtk_combo_box_text_insert(GTK_COMBO_BOX_TEXT(menu),p,scharset,text);

#else

		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(menu), renderer, TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(menu), renderer, "text", 0, NULL);

		gtk_list_store_append((GtkListStore *) model,&iter);
		gtk_list_store_set((GtkListStore *) model, &iter, 0, text, 1, scharset, -1);

#endif // GTK(3,0,0)

		g_free(text);

		gtk_combo_box_set_active(GTK_COMBO_BOX(menu),p++);

		for(f=0;charset[f].name;f++)
		{
			if(strcasecmp(scharset,charset[f].name))
			{
#if GTK_CHECK_VERSION(3,0,0)
				gtk_combo_box_text_insert(GTK_COMBO_BOX_TEXT(menu),p++,charset[f].name,gettext(charset[f].description));
#else
				gtk_list_store_append((GtkListStore *) model,&iter);
				gtk_list_store_set((GtkListStore *) model, &iter, 0, gettext(charset[f].description), 1, charset[f].name, -1);
#endif // GTK(3,0,0)
			}
		}


		gtk_label_set_mnemonic_widget(GTK_LABEL(label), menu);

		gtk_box_pack_start(GTK_BOX(box),label,FALSE,FALSE,0);

		gtk_box_pack_start(GTK_BOX(box),menu,TRUE,TRUE,0);

		g_signal_connect(G_OBJECT(menu),"changed",G_CALLBACK(charset_changed),encoding);

	}


	gtk_widget_show_all(box);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(widget),box);

}

 static void save_text(GtkWindow *toplevel,const gchar *filename, const gchar *text, const gchar *encoding, const gchar *errmsg)
 {
	GError * error = NULL;

	if(g_ascii_strcasecmp(encoding,"UTF-8"))
	{
		// Convert to target charset and save
		gsize	  bytes_written;
		gchar	* converted = g_convert_with_fallback(text,-1,encoding,"UTF-8",NULL,NULL,&bytes_written,&error);

		if(!error)
			g_file_set_contents(filename,converted,-1,&error);

		g_free(converted);
	}
	else
	{
		// Same charset, save file
		g_file_set_contents(filename,text,-1,&error);
	}

	if(error)
	{
		GtkWidget *popup = gtk_message_dialog_new_with_markup(
											toplevel,
											GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
											GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE,
											gettext(errmsg),filename);

		gtk_window_set_title(GTK_WINDOW(popup),_("Can't save file"));

		gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(popup),"%s",error->message);
		g_error_free(error);

		gtk_dialog_run(GTK_DIALOG(popup));
		gtk_widget_destroy(popup);

	}

 }

 static GtkFileChooserConfirmation confirm_overwrite(GtkFileChooser *chooser, GObject *action)
 {
	const gchar					* attr		= g_object_get_data(action,"overwrite");
	GtkFileChooserConfirmation	  ret 		= GTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME;
	GtkWidget					* dialog;

	if(attr && !g_ascii_strcasecmp(attr,"yes"))
		return ret;

	dialog = gtk_message_dialog_new_with_markup(	GTK_WINDOW(chooser),
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_MESSAGE_QUESTION,GTK_BUTTONS_OK_CANCEL,
													"%s",_("The file already exists. Replace it?"));


	if(gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
		ret = GTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN;

	gtk_widget_destroy(dialog);

	return ret;

 }

 static int save_dialog(GtkAction *action, GtkWidget *widget, const gchar *title, const gchar *errmsg, const gchar *text)
 {
 	GtkWindow	* toplevel		= GTK_WINDOW(gtk_widget_get_toplevel(widget));
 	const gchar * user_title	= g_object_get_data(G_OBJECT(action),"title");
 	const gchar * filename		= g_object_get_data(G_OBJECT(action),"filename");
 	const gchar * extension		= g_object_get_data(G_OBJECT(action),"extension");

	if(!extension)
		extension = "txt";

	if(!text)
		return 0;

	if(filename)
	{
		save_text(toplevel,filename,text,g_object_get_data(G_OBJECT(action),"encoding"),errmsg);
	}
	else
	{
		GtkWidget	* dialog;
		gchar		* ptr;
		gchar 		* encattr		= NULL;

		dialog = gtk_file_chooser_dialog_new( 	gettext(user_title ? user_title : title),
												toplevel,
												GTK_FILE_CHOOSER_ACTION_SAVE,
												GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
												GTK_STOCK_SAVE,		GTK_RESPONSE_ACCEPT,
												NULL );

		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
		g_signal_connect(GTK_FILE_CHOOSER(dialog), "confirm-overwrite", G_CALLBACK(confirm_overwrite), G_OBJECT(action));

		add_option_menus(dialog, action, &encattr);

		ptr = get_string_from_config("save",gtk_action_get_name(action),"");
		if(*ptr)
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),ptr);
		else
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS));
		g_free(ptr);

		if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			ptr = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			if(ptr)
			{
				trace("Saving \"%s\"",ptr);
				set_string_to_config("save",gtk_action_get_name(action),"%s",ptr);
				save_text(toplevel,ptr,text,encattr,errmsg);
				g_free(ptr);
			}
		}

		if(encattr)
			g_free(encattr);

		trace("Removing dialog %p",dialog);
		gtk_widget_destroy(dialog);
	}

	return 0;
 }

 void save_all_action(GtkAction *action, GtkWidget *widget)
 {
 	gchar *text = v3270_get_text(widget,0,-1);

	trace("Action %s activated on widget %p text=%p",gtk_action_get_name(action),widget,text);

	if(!text)
	{
		g_warning("%s","Buffer contents was NULL");
		return;
	}

	save_dialog(	action,
					widget,
					N_( "Save screen to file" ),
					N_( "Can't save screen to file\n%s" ),
					text);

	g_free(text);

 }

 void save_selected_action(GtkAction *action, GtkWidget *widget)
 {
    gchar *text = v3270_get_selected(widget,FALSE);

	trace("Action %s activated on widget %p",gtk_action_get_name(action),widget);

    if(text)
    {
        save_dialog(	action,
                        widget,
                        N_( "Save selection to file" ),
                        N_( "Can't save selection to file\n%s" ),
                        text);
        g_free(text);
    }
 }

 void save_copy_action(GtkAction *action, GtkWidget *widget)
 {
    gchar *text = v3270_get_copy(widget);

	trace("Action %s activated on widget %p",gtk_action_get_name(action),widget);

    if(text)
    {
        save_dialog(	action,
                        widget,
                        N_( "Save copy to file" ),
                        N_( "Can't save copy to file\n%s" ),
                        text);
        g_free(text);
    }
 }

 static void paste_filename(GtkWidget *widget, const gchar *filename, const gchar *encoding)
 {
	GError *error = NULL;
	gchar *text = NULL;

	if(!encoding)
		g_get_charset(&encoding);

	trace("Loading \"%s\" encoding=%s",filename,encoding);

	if(!g_file_get_contents(filename,&text,NULL,&error))
	{
		GtkWidget *popup = gtk_message_dialog_new_with_markup(
											GTK_WINDOW(gtk_widget_get_toplevel(widget)),
											GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
											GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE,
											_( "Error loading %s" ),filename);

		gtk_window_set_title(GTK_WINDOW(popup),_("Can't load file"));

		gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(popup),"%s",error->message);
		g_error_free(error);

		gtk_dialog_run(GTK_DIALOG(popup));
		gtk_widget_destroy(popup);

	}

	if(text)
	{
		v3270_paste_string(widget,text,encoding);
		g_free(text);
	}

 }

 void paste_file_action(GtkAction *action, GtkWidget *widget)
 {
 	const gchar * user_title	= g_object_get_data(G_OBJECT(action),"title");
 	const gchar * filename		= g_object_get_data(G_OBJECT(action),"filename");
 	gchar 		* encattr		= NULL;
	GtkWidget	* dialog;
	gchar		* ptr;

	trace("Action %s activated on widget %p",gtk_action_get_name(action),widget);

	if(filename)
	{
		ptr = g_object_get_data(G_OBJECT(action),"charset");
		paste_filename(widget,filename,ptr);
		return;
	}

	dialog = gtk_file_chooser_dialog_new( 	gettext(user_title ? user_title : N_( "Paste text file contents" )),
											GTK_WINDOW(gtk_widget_get_toplevel(widget)),
											GTK_FILE_CHOOSER_ACTION_OPEN,
											GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
											GTK_STOCK_OPEN,		GTK_RESPONSE_ACCEPT,
											NULL );

	add_option_menus(dialog, action, &encattr);

	ptr = get_string_from_config("load",gtk_action_get_name(action),"");

	if(*ptr)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),ptr);
	else
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS));

	g_free(ptr);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		ptr = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		if(ptr)
		{
			set_string_to_config("load",gtk_action_get_name(action),"%s",ptr);
			paste_filename(widget,ptr,encattr);
			g_free(ptr);
		}
	}
	gtk_widget_destroy(dialog);

	if(encattr)
		g_free(encattr);
 }

 G_GNUC_INTERNAL void about_dialog_action(GtkAction *action, GtkWidget *widget)
 {
 	static const gchar *authors[]	= {	"Perry Werneck <perry.werneck@gmail.com>",
										"Paul Mattes <Paul.Mattes@usa.net>",
										"Georgia Tech Research Corporation (GTRC)",
										"and others",
										NULL };

	static const gchar *license		=
	N_( "This program is free software; you can redistribute it and/or "
		"modify it under the terms of the GNU General Public License as "
 		"published by the Free Software Foundation; either version 2 of the "
		"License, or (at your option) any later version.\n\n"
		"This program is distributed in the hope that it will be useful, "
		"but WITHOUT ANY WARRANTY; without even the implied warranty of "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
		"GNU General Public License for more details.\n\n"
		"You should have received a copy of the GNU General Public License "
		"along with this program; if not, write to the Free Software "
		"Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02111-1307 "
		"USA" );

	GtkAboutDialog  * dialog 	= GTK_ABOUT_DIALOG(gtk_about_dialog_new());
	gchar			* text 		= g_strdup_printf("%s-logo.png",g_get_application_name());
	gchar			* filename	= build_data_filename(text,NULL);
	gchar			* info;

	trace("[%s]",filename);
	if(g_file_test(filename,G_FILE_TEST_EXISTS))
	{
		GError 		* error	= NULL;
		GdkPixbuf	* pix	= gdk_pixbuf_new_from_file(filename,&error);

		gtk_about_dialog_set_logo(dialog,pix);

		if(pix)
		{
			g_object_unref(pix);
		}
		else
		{
			g_warning("Can't load %s: %s",filename,error->message);
			g_error_free(error);
		}
	}

	g_free(filename);
	g_free(text);

#ifdef PACKAGE_RELEASE
	text = g_strdup_printf(_("Version %s-%s"),PACKAGE_VERSION,G_STRINGIFY(PACKAGE_RELEASE));
#else
	text = g_strdup_printf(_("Version %s-%s"),PACKAGE_VERSION,G_STRINGIFY(BUILD_DATE));
#endif // PACKAGE_REVISION

	gtk_about_dialog_set_version(dialog,text);
	g_free(text);

	gtk_about_dialog_set_copyright(dialog, "Copyright © 2008 Banco do Brasil S.A." );

	info = g_strdup_printf(_( "3270 terminal emulator for GTK %d.%d" ),GTK_MAJOR_VERSION,GTK_MINOR_VERSION);
	gtk_about_dialog_set_comments(dialog, info );

	gtk_about_dialog_set_license(dialog, gettext( license ) );
	gtk_about_dialog_set_wrap_license(dialog,TRUE);

	gtk_about_dialog_set_website(dialog,"https://portal.softwarepublico.gov.br/social/pw3270/");
	gtk_about_dialog_set_website_label(dialog,_( "Brazilian Public Software Portal" ));

	gtk_about_dialog_set_authors(dialog,authors);
	gtk_about_dialog_set_translator_credits(dialog,_("translator-credits"));

	gtk_widget_show_all(GTK_WIDGET(dialog));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));

	g_free(info);
 }

#ifdef WIN32
static gpointer select_file(struct file *fl) {


	switch(fl->action) {
	case GTK_FILE_CHOOSER_ACTION_SAVE:	// Receber arquivo
										// https://msdn.microsoft.com/en-us/library/windows/desktop/ms646839(v=vs.85).aspx
										// https://msdn.microsoft.com/en-us/library/windows/desktop/ms646829(v=vs.85).aspx#open_file
		fl->ofn.Flags = OFN_OVERWRITEPROMPT | OFN_CREATEPROMPT | OFN_HIDEREADONLY;
		fl->ok = GetSaveFileName(&fl->ofn);
		break;

	case GTK_FILE_CHOOSER_ACTION_OPEN:	// Enviar arquivo
										// https://msdn.microsoft.com/en-us/library/windows/desktop/ms646928(v=vs.85).aspx
		fl->ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		fl->ok = GetOpenFileName(&fl->ofn);
		break;
	}

	fl->enabled = FALSE;

	return 0;
}
#endif // _WIN32

 LIB3270_EXPORT gchar * pw3270_file_chooser(GtkFileChooserAction action, const gchar *name,  const gchar *title, const gchar *file, const gchar *ext)
 {
    gchar       * filename      = NULL;

#if defined(_WIN32)

	GThread 	* thd;
	struct file	  fl;
	GtkWidget	* dialog	= pw3270_get_toplevel();
	GdkWindow	* win		= gtk_widget_get_window(GTK_WIDGET(dialog));

	gtk_widget_set_sensitive(GTK_WIDGET(dialog),FALSE);

	memset(&fl,0,sizeof(fl));
	fl.ofn.lStructSize		= sizeof(fl.ofn);
	fl.action				= action;
	fl.ofn.hwndOwner		= GDK_WINDOW_HWND(win);
	fl.ofn.lpstrFile		= fl.szName;
	fl.ofn.lpstrTitle		= title;

	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not
	// use the contents of szFile to initialize itself.
	fl.ofn.lpstrFile[0] 	= '\0';
	fl.ofn.nMaxFile 		= sizeof(fl.szName);

	fl.ofn.lpstrInitialDir	= NULL;
	fl.ofn.nMaxFileTitle	= 0;

	// Guarda o valor atual
    if(file && *file)
    {
		strncpy(fl.szName,file,fl.ofn.nMaxFile);
    }
    else
    {
        gchar *ptr = get_string_from_config("files",name,"");
        if(*ptr)
			strncpy(fl.szName,ptr,fl.ofn.nMaxFile);
        else
			fl.ofn.lpstrInitialDir = g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS);
        g_free(ptr);
    }

	thd = g_thread_new("GetFileName",(GThreadFunc) select_file, &fl);

	fl.enabled = TRUE;
	while(fl.enabled) {
		g_main_context_iteration(NULL,TRUE);
	}

	g_thread_unref(thd);

	if(fl.ok) {
		filename = g_strdup(fl.szName);
	}

	gtk_widget_set_sensitive(GTK_WIDGET(dialog),TRUE);

#else
    static const struct _btn
    {
            const gchar          * button;
            GtkFileChooserAction   action;
    } btn[] =
    {
        { GTK_STOCK_OPEN,     GTK_FILE_CHOOSER_ACTION_OPEN            },
        { GTK_STOCK_SAVE,     GTK_FILE_CHOOSER_ACTION_SAVE            },
        { GTK_STOCK_OK,       GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER   },
        { GTK_STOCK_OK,       GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER   }
    };

    GtkWidget	* dialog;
    gchar		* ptr;
    const gchar * button        = GTK_STOCK_OK;
    int           f;

    for(f=0;f<G_N_ELEMENTS(btn);f++)
    {
        if(action == btn[f].action)
        {
            button = btn[f].button;
            break;
        }
    }

    dialog = gtk_file_chooser_dialog_new( 	title,
                                            GTK_WINDOW(pw3270_get_toplevel()),
                                            action,
                                            GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
                                            button,		        GTK_RESPONSE_ACCEPT,
                                            NULL );

    if(action == GTK_FILE_CHOOSER_ACTION_SAVE)
    {
        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
        g_signal_connect(GTK_FILE_CHOOSER(dialog), "confirm-overwrite", G_CALLBACK(confirm_overwrite), G_OBJECT(dialog));
    }

    if(file && *file)
    {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),file);
    }
    else
    {
        ptr = get_string_from_config("files",name,"");
        if(*ptr)
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),ptr);
        else
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS));
        g_free(ptr);
    }

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }

    gtk_widget_destroy(dialog);
#endif // WIN32

	if(filename && *filename)
		set_string_to_config("files",name,"%s",filename);

    return filename;
 }

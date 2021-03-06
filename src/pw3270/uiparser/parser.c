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
 * Este programa está nomeado como parser.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 *
 */

 #include "private.h"
 #include <v3270.h>

 #ifdef HAVE_GTKMAC
	#include <gtkmacintegration/gtk-mac-menu.h>
 #endif // HAVE_GTKMAC

/*--[ Globals ]--------------------------------------------------------------------------------------*/

static const gchar	* dirname[]	= { "up", "down", "left", "right", "end" };

/*--[ Implement ]------------------------------------------------------------------------------------*/

void parser_init(struct parser *p)
{
	int f;

	memset(p,0,sizeof(struct parser));
	p->actions = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) g_object_unref);

	for(f=0;f<UI_ELEMENT_COUNT;f++)
		p->element_list[f] = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) g_object_unref);

	trace("%s ends",__FUNCTION__);
}

void parser_destroy(struct parser *p)
{
	int f;

	g_hash_table_unref(p->actions);

	for(f=0;f<UI_ELEMENT_COUNT;f++)
		g_hash_table_unref(p->element_list[f]);

	trace("%s ends",__FUNCTION__);
}

static void pack_start(gpointer key, GtkWidget *widget, struct parser *p)
{
 	trace("Packing %s - %p",gtk_widget_get_name(widget),widget);
	gtk_box_pack_start(GTK_BOX(p->element),widget,FALSE,FALSE,0);
}


struct keypad
{
	GtkWidget		* box;
	GtkPositionType	  filter;
	void              (*pack)(GtkBox *,GtkWidget *, gboolean,gboolean,guint);
};

static void pack_keypad(gpointer key, GtkWidget *widget, struct keypad *k)
{
	if((GtkPositionType) g_object_get_data(G_OBJECT(widget),"position") != k->filter) {
		return;
	}

	trace("%s %s",__FUNCTION__,gtk_widget_get_name(widget));
	k->pack(GTK_BOX(k->box),widget,FALSE,FALSE,0);

}

static void pack_view(gpointer key, GtkWidget *widget, GtkWidget *parent)
{
	GObject *obj = g_object_get_data(G_OBJECT(widget),"view_action");

	if(obj && GTK_IS_ACTION(obj))
	{
		GtkWidget * menu	= parent;
		gboolean	visible	= get_boolean_from_config("view",gtk_action_get_name(GTK_ACTION(obj)),TRUE);

#if GTK_CHECK_VERSION(2,18,0)
		gtk_widget_set_visible(widget,visible);
#else
		if(visible)
			gtk_widget_show(widget);
		else
			gtk_widget_hide(widget);
#endif // GTK(2,18,0)

		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(obj),visible);

		if(GTK_IS_MENU_ITEM(menu))
		{
			menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(parent));
			if(!menu)
			{
				menu = gtk_menu_new();
				gtk_menu_item_set_submenu(GTK_MENU_ITEM(GTK_MENU_ITEM(parent)),menu);
			}
		}

		gtk_menu_shell_append((GtkMenuShell *) menu, gtk_action_create_menu_item(GTK_ACTION(obj)));

	}

}

struct action_info
{
	GtkActionGroup	** group;
	GtkAccelGroup	*  accel_group;
	GtkWidget		*  widget;
};

static void action_group_setup(gpointer key, GtkAction *action, struct action_info *info)
{
	int group_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action),"id_group"));

	gtk_action_set_accel_group(action,info->accel_group);

	if(group_id >= 0)
	{
		const gchar	* key_name = g_object_get_data(G_OBJECT(action),"accel_attr");
		GSList		* child = gtk_action_get_proxies(action);

		if(key_name && !v3270_set_keyboard_action(info->widget,key_name,action))
		{
			gtk_action_group_add_action_with_accel(info->group[group_id],action,key_name);
			gtk_action_connect_accelerator(action);
		}
		else
		{
			gtk_action_group_add_action(info->group[group_id],action);
		}

		// Update proxy widgets
#if GTK_CHECK_VERSION(2,16,0)
        while(child)
        {
			gtk_activatable_sync_action_properties(GTK_ACTIVATABLE(child->data),action);
			child = child->next;
        }
#else

#endif // GTK(2,16,0)

	}

	g_object_set_data(G_OBJECT(action),"id_group",		NULL);
	g_object_set_data(G_OBJECT(action),"accel_attr",	NULL);
}

static void release_action_group(GtkActionGroup	** group)
{
	int f;

	for(f=0;group[f];f++)
		g_object_unref(group[f]);

	g_free(group);
}

void parser_build(struct parser *p, GtkWidget *widget)
{
	struct action_info	  a_info;

#if GTK_CHECK_VERSION(3,0,0)
	GtkWidget			* mainBox	= gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	GtkWidget			* vbox		= gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	GtkWidget			* hbox		= gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
#else
	GtkWidget			* mainBox	= gtk_vbox_new(FALSE,0);
	GtkWidget			* vbox		= gtk_vbox_new(FALSE,0);
	GtkWidget			* hbox		= gtk_hbox_new(FALSE,0);
#endif // GTK(3,0,0)

	GtkWidget			* parent;
	struct keypad		  keypad;
	int				 	  f;

	a_info.widget		= widget;
	a_info.group 		= g_new0(GtkActionGroup *,(g_strv_length((gchar **) p->group)+1));
	a_info.accel_group	= gtk_accel_group_new();

	for(f=0;p->group[f];f++)
		a_info.group[f] = gtk_action_group_new(p->group[f]);

	g_object_set_data_full(G_OBJECT(p->toplevel),"action_groups",a_info.group,(GDestroyNotify) release_action_group);
	g_object_set_data_full(G_OBJECT(p->toplevel),"string_chunk",p->strings,(GDestroyNotify) g_string_chunk_free);

	g_signal_connect(p->toplevel, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	// Has "view" menu? Update it.
	parent = GTK_WIDGET(g_hash_table_lookup(p->element_list[UI_ELEMENT_MENU],"view"));
	if(parent)
	{
		g_hash_table_foreach(p->element_list[UI_ELEMENT_TOOLBAR],(GHFunc) pack_view, parent);
		g_hash_table_foreach(p->element_list[UI_ELEMENT_KEYPAD],(GHFunc) pack_view, parent);
	}

	// Setup actions
	g_hash_table_foreach(p->actions,(GHFunc) action_group_setup, &a_info);

	// Pack menubars
	p->element = G_OBJECT(mainBox);
	g_hash_table_foreach(p->element_list[UI_ELEMENT_MENUBAR],(GHFunc) pack_start, p);

	// Pack top toolbars
	g_hash_table_foreach(p->element_list[UI_ELEMENT_TOOLBAR],(GHFunc) pack_start, p);

	// Setup keypads
	ui_setup_keypads(p->element_list[UI_ELEMENT_KEYPAD], p->toplevel, p->center_widget);

	// Pack top keypads
	trace("Packing %s keypads","top");
	memset(&keypad,0,sizeof(keypad));
	keypad.box 		= vbox;
	keypad.filter 	= GTK_POS_TOP;
	keypad.pack 	= gtk_box_pack_start;
	g_hash_table_foreach(p->element_list[UI_ELEMENT_KEYPAD],(GHFunc) pack_keypad, &keypad);

	// Pack left keypads
	trace("Packing %s keypads","left");
	keypad.box 		= hbox;
	keypad.filter 	= GTK_POS_LEFT;
	g_hash_table_foreach(p->element_list[UI_ELEMENT_KEYPAD],(GHFunc) pack_keypad, &keypad);

	// Pack & configure center widget
	if(widget)
	{
		ui_set_scroll_actions(widget,p->scroll_action);
		gtk_box_pack_start(GTK_BOX(vbox),widget,TRUE,TRUE,0);
		gtk_widget_show(widget);
	}
	gtk_box_pack_start(GTK_BOX(hbox),vbox,TRUE,TRUE,0);

	// Pack right keypads
	trace("Packing %s keypads","right");
	keypad.filter 	= GTK_POS_RIGHT;
	keypad.pack 	= gtk_box_pack_end;
	g_hash_table_foreach(p->element_list[UI_ELEMENT_KEYPAD],(GHFunc) pack_keypad, &keypad);

	// Pack bottom keypads
	trace("Packing %s keypads","bottom");
	keypad.box 		= vbox;
	keypad.filter 	= GTK_POS_BOTTOM;
	g_hash_table_foreach(p->element_list[UI_ELEMENT_KEYPAD],(GHFunc) pack_keypad, &keypad);

	// Finish building
	gtk_box_pack_start(GTK_BOX(mainBox),hbox,TRUE,TRUE,0);
	gtk_container_add(GTK_CONTAINER(p->toplevel),mainBox);

	gtk_widget_show(hbox);
	gtk_widget_show(vbox);
	gtk_widget_show(mainBox);

	gtk_window_add_accel_group(GTK_WINDOW(p->toplevel),a_info.accel_group);

	gtk_window_set_default(GTK_WINDOW(p->toplevel),widget);

#ifdef HAVE_GTKMAC
	{
	    if(p->sysmenu[SYSMENU_ITEM_QUIT])
	    {
		gtk_mac_menu_set_quit_menu_item (GTK_MENU_ITEM(p->sysmenu[SYSMENU_ITEM_QUIT]));
	    }

	    if(p->sysmenu[SYSMENU_ITEM_ABOUT])
	    {
		GtkMacMenuGroup *group = gtk_mac_menu_add_app_menu_group();
		gtk_mac_menu_add_app_menu_item(group,GTK_MENU_ITEM(p->sysmenu[SYSMENU_ITEM_ABOUT]),NULL);
	    }

	    if(p->sysmenu[SYSMENU_ITEM_PREFERENCES])
	    {
		GtkMacMenuGroup *group	= gtk_mac_menu_add_app_menu_group();
		gtk_widget_show_all(p->sysmenu[SYSMENU_ITEM_PREFERENCES]);
		gtk_mac_menu_add_app_menu_item(group,GTK_MENU_ITEM(p->sysmenu[SYSMENU_ITEM_PREFERENCES]),NULL);
	    }

	    if(p->topmenu)
	    {
#if GTK_CHECK_VERSION(2,18,0)
		    gtk_widget_set_visible(p->topmenu,FALSE);
#else
		    gtk_widget_hide(p->topmenu);
#endif // GTK(2,18,0)
		    gtk_mac_menu_set_menu_bar(GTK_MENU_SHELL(p->topmenu));
	    }

	}
#endif // HAVE_GTKMAC

}

static void release_list(GObject **obj)
{
	int f;
	for(f=0;obj[f] != ((GObject *) -1);f++)
	{
		if(obj[f])
			g_object_unref(obj[f]);
	}
	g_free(obj);
}

int ui_parse_xml_folder(GtkWindow *toplevel, const gchar *path, const gchar ** groupname, const gchar **popupname, GtkWidget *widget, const UI_WIDGET_SETUP *setup)
{
	struct parser	  p;
	GDir 			* dir;
	GError			* error		= NULL;
	gchar			* ptr;
	GList			* file		= NULL;
	GList			* current;
	size_t			  sz;

	dir = g_dir_open(path,0,&error);

	if(!dir)
	{
		GtkWidget * dialog = gtk_message_dialog_new(
									NULL,
									GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_MESSAGE_WARNING,
									GTK_BUTTONS_OK,
									_(  "Can't parse UI description files in %s" ), path);

		gtk_window_set_title(GTK_WINDOW(dialog), _( "Can't parse UI" ) );

		if(error && error->message)
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", error->message);

		g_error_free(error);

		gtk_dialog_run(GTK_DIALOG (dialog));
		gtk_widget_destroy(dialog);

		return -1;
	}

	while( (ptr = (gchar *) g_dir_read_name(dir)) != NULL)
	{
		gchar *filename = g_build_filename(path,ptr,NULL);

		if(*ptr != '.' && g_file_test(filename,G_FILE_TEST_IS_REGULAR) && g_str_has_suffix(filename,".xml"))
			file = g_list_insert_sorted(file,filename,(GCompareFunc) g_strcmp0);
		else
			g_free(filename);
	}

	g_dir_close(dir);

	parser_init(&p);
	p.toplevel 		= GTK_WIDGET(toplevel);
	p.center_widget	= widget;
	p.group 		= groupname;
	p.popupname		= popupname;
	p.strings		= g_string_chunk_new(0);
	p.setup			= setup;

	sz				= (g_strv_length((gchar **) p.popupname));
	p.popup 		= g_new0(GtkWidget *,sz+1);
	p.popup[sz]		= (GtkWidget *) -1;
	g_object_set_data_full(G_OBJECT(p.toplevel),"popup_menus",(gpointer) p.popup, (GDestroyNotify) release_list);

	for(current = g_list_first(file);current;current = g_list_next(current))
	{
		ui_parse_file(&p,(gchar *) current->data);
		g_free(current->data);
	}
	g_list_free(file);

	parser_build(&p,widget);

	if(setup)
	{
		int f;

		for(f=0;setup[f].name;f++)
		{
			int i;
			for(i=0;i<UI_ELEMENT_COUNT;i++)
			{
				GObject * obj = g_hash_table_lookup(p.element_list[i],setup[f].name);
				if(obj && GTK_IS_WIDGET(obj))
					setup[f].setup(GTK_WIDGET(obj),p.center_widget);
			}
		}
	}

	parser_destroy(&p);

	return 0;
}

UI_ATTR_DIRECTION ui_get_dir_attribute(const gchar **names, const gchar **values)
{
	const gchar			* dir		= ui_get_attribute("direction",names,values);
	int					  f;

	if(dir)
	{
		for(f=0;f<G_N_ELEMENTS(dirname);f++)
		{
			if(!g_ascii_strcasecmp(dir,dirname[f]))
				return f;
		}
	}

	return UI_ATTR_DIRECTION_NONE;
}

const gchar * ui_get_dir_name(UI_ATTR_DIRECTION dir)
{
	if(dir == UI_ATTR_DIRECTION_NONE)
		return "";
	return dirname[dir];
}

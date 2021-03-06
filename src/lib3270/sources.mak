#
# "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
# (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
# aplicativos mainframe. Registro no INPI sob o nome G3270.
#
# Copyright (C) <2008> <Banco do Brasil S.A.>
#
# Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
# os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
# Free Software Foundation.
#
# Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
# GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
# A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
# obter mais detalhes.
#
# Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
# programa;  se  não, escreva para a Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA, 02111-1307, USA
#
# Contatos:
#
# perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
# erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
#

# Terminal only sources
TERMINAL_SOURCES =	bounds.c ctlr.c util.c toggles.c screen.c selection.c kybd.c telnet.c \
				host.c sf.c ansi.c resolver.c charset.c \
				version.c session.c state.c html.c trace_ds.c see.c \
				paste.c ssl.c actions.c

# tables.c utf8.c

# Network I/O Sources
NETWORK_SOURCES = iocalls.c connect.c

# Full library sources
SOURCES =			$(TERMINAL_SOURCES) $(NETWORK_SOURCES) ft.c ft_cut.c ft_dft.c glue.c resources.c \
					rpq.c macros.c fallbacks.c log.c options.c


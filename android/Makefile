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
#

#
# Referência:
# http://developer.android.com/tools/building/building-cmdline.html
#

JDK_HOME=/usr/java/default
JAVAH=$(JDK_HOME)/bin/javah
JAVAC=$(JDK_HOME)/bin/javac
ANT=/usr/bin/ant

ANDROIDSDK?=/opt/android-sdk-linux
ANDROIDNDK?=/opt/android-ndk-r8d
ANDROIDCLASSPATH=$(ANDROIDSDK)/platforms/android-16/android.jar

NDKBUILD=$(ANDROIDNDK)/ndk-build
MKDIR=/bin/mkdir -p
CONVERT=rsvg-convert

CLASSPATH=br/com/bb/pw3270
JAVASRCDIR=src/$(CLASSPATH)

RESOLUTION=xhdpi mdpi ldpi hdpi

#---[ Rules ]------------------------------------------------------------------

bin/classes/$(CLASSPATH)/%.class: src/$(CLASSPATH)/%.java
	@$(MKDIR) `dirname $@`
	@echo "Compile thumb  : `basename $@` <= `basename $<`"
	@$(JAVAC) -classpath $(ANDROIDCLASSPATH) -d bin/classes $<

res/drawable-xhdpi/%.png: %.svg
	@echo "Gen XHDPI      : `basename $@` <= `basename $<`"
	@$(MKDIR) `dirname $@`
	@$(CONVERT) --format=png --width=96 --height=96 --output=$@ $<

res/drawable-mdpi/%.png: %.svg
	@echo "Gen MDPI       : `basename $@` <= `basename $<`"
	@$(MKDIR) `dirname $@`
	@$(CONVERT) --format=png --width=48 --height=48 --output=$@ $<

res/drawable-ldpi/%.png: %.svg
	@echo "Gen LDPI       : `basename $@` <= `basename $<`"
	@$(MKDIR) `dirname $@`
	@$(CONVERT) --format=png --width=36 --height=36 --output=$@ $<

res/drawable-hdpi/%.png: %.svg
	@echo "Gen HDPI       : `basename $@` <= `basename $<`"
	@$(MKDIR) `dirname $@`
	@$(CONVERT) --format=png --width=72 --height=72 --output=$@ $<

#---[ Targets ]----------------------------------------------------------------

all: pw3270-release-unsigned.apk

Debug: bin/pw3270-debug.apk

bin/pw3270-debug.apk: icons libs/armeabi/lib3270.so $(JAVASRCDIR)/*.java
	@JAVA_HOME=$(JDK_HOME) $(ANT) debug

pw3270-release-unsigned.apk: icons libs/armeabi/lib3270.so $(JAVASRCDIR)/*.java
	@JAVA_HOME=$(JDK_HOME) $(ANT) release

libs/armeabi/lib3270.so: jni/lib3270jni.h jni/*.cpp
	@echo "  GEN  $@"
	@chmod +x ../src/lib3270/mkversion.sh
	@../src/lib3270/mkversion.sh
	@$(NDKBUILD)

icons: $(foreach DIR, $(RESOLUTION), res/drawable-$(DIR)/ic_launcher.png)
	
clean:
	@JAVA_HOME=$(JDK_HOME) $(ANT) clean
	@rm -fr bin
	@rm -fr obj
	@rm -f jni/lib3270jni.h
	@rm -f sigs.txt
	@rm -f $(foreach DIR, $(RESOLUTION), res/drawable-$(DIR)/*.png)

cleanDebug: clean

res/drawable-xhdpi/ic_launcher.png: ../src/pw3270/pixmaps/pw3270.svg
	@echo "Laucher xhdpi  : `basename $@` <= `basename $<`"
	@$(MKDIR) `dirname $@`
	@$(CONVERT) --format=png --width=96 --height=96 --output=$@ $<

res/drawable-mdpi/ic_launcher.png: ../src/pw3270/pixmaps/pw3270.svg
	@echo "Laucher mdpi   : `basename $@` <= `basename $<`"
	@$(MKDIR) `dirname $@`
	@$(CONVERT) --format=png --width=48 --height=48 --output=$@ $<

res/drawable-ldpi/ic_launcher.png: ../src/pw3270/pixmaps/pw3270.svg
	@echo "Laucher ldpi   : `basename $@` <= `basename $<`"
	@$(MKDIR) `dirname $@`
	@$(CONVERT) --format=png --width=36 --height=36 --output=$@ $<

res/drawable-hdpi/ic_launcher.png: ../src/pw3270/pixmaps/pw3270.svg
	@echo "Laucher hdpi   : `basename $@` <= `basename $<`"
	@$(MKDIR) `dirname $@`
	@$(CONVERT) --format=png --width=72 --height=72 --output=$@ $<

sigs.txt: bin/classes/$(CLASSPATH)/lib3270.class
	@javap -classpath ./bin/classes -s -p br.com.bb.pw3270.lib3270 > $@

jni/lib3270jni.h: bin/classes/$(CLASSPATH)/lib3270.class
	@echo "  GEN  `basename $@`"
	@$(JAVAH) -o $@ -classpath $(ANDROIDCLASSPATH):bin/classes $(subst /,.,$(CLASSPATH)).lib3270

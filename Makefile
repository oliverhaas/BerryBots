##############################################################################
# Configure build parameters.
CC = g++
.DEFAULT_GOAL = linux

SFML_HEADER_PATH = 
SFML_LIB_PATH = 
WXWIDGETS_PATH = 
LIBARCHIVE_PATH = 

LOCAL_SFML=0
LOCAL_WX=0
LOCAL_LIBARCHIVE=0

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
datarootdir = ${prefix}/share
datadir = ${datarootdir}
##############################################################################

# You shouldn't have to edit anything below this line. #######################

VERSION = 1.3.3

##############################################################################
# BerryBots GUI common source files
SOURCES =  bbguimain.cpp guiprinthandler.cpp guimanager.cpp newmatch.cpp
SOURCES += outputconsole.cpp bbutil.cpp gfxmanager.cpp filemanager.cpp
SOURCES += circle2d.cpp line2d.cpp point2d.cpp sensorhandler.cpp zone.cpp
SOURCES += bbengine.cpp bblua.cpp gfxeventhandler.cpp rectangle.cpp stage.cpp
SOURCES += packagedialog.cpp packageship.cpp packagestage.cpp dockitem.cpp
SOURCES += dockshape.cpp docktext.cpp dockfader.cpp zipper.cpp guizipper.cpp
SOURCES += menubarmaker.cpp guigamerunner.cpp runnerdialog.cpp runnerform.cpp
SOURCES += bbrunner.cpp resultsdialog.cpp replaybuilder.cpp sysexec.cpp
SOURCES += stagepreview.cpp
##############################################################################


##############################################################################
# Sources and flags for building on Raspberry Pi (Raspbian "wheezy")
RPI_SOURCES =  bbpimain.cpp bbengine.cpp stage.cpp bbutil.cpp bblua.cpp
RPI_SOURCES += line2d.cpp point2d.cpp circle2d.cpp rectangle.cpp zone.cpp
RPI_SOURCES += bbpigfx.cpp filemanager.cpp gfxeventhandler.cpp sensorhandler.cpp
RPI_SOURCES += cliprinthandler.cpp clipackagereporter.cpp libshapes.c oglinit.c
RPI_SOURCES += zipper.cpp tarzipper.cpp bbrunner.cpp relativebasedir.cpp
RPI_SOURCES += relativerespath.cpp replaybuilder.cpp ./luajit/src/libluajit.a

RPI_CFLAGS =  -I./luajit/src -I./stlsoft-1.9.116/include -I/opt/vc/include
RPI_CFLAGS += -I/opt/vc/include/interface/vcos/pthreads
RPI_CFLAGS += -I/opt/vc/include/interface/vmcs_host/linux

RPI_LDFLAGS = -L/opt/vc/lib -lGLESv2 -lEGL -lbcm_host -lpthread -ldl
##############################################################################


##############################################################################
# BerryBots CLI common source files
CLI_SOURCES =  bbsfmlmain.cpp bbutil.cpp gfxmanager.cpp filemanager.cpp
CLI_SOURCES += circle2d.cpp line2d.cpp point2d.cpp sensorhandler.cpp zone.cpp
CLI_SOURCES += bbengine.cpp bblua.cpp gfxeventhandler.cpp rectangle.cpp
CLI_SOURCES += stage.cpp cliprinthandler.cpp clipackagereporter.cpp dockitem.cpp
CLI_SOURCES += dockshape.cpp docktext.cpp dockfader.cpp zipper.cpp guizipper.cpp
CLI_SOURCES += bbrunner.cpp replaybuilder.cpp
##############################################################################


##############################################################################
# BerryBots webui source files
WEBUI_SOURCES =  bbwebmain.cpp bbutil.cpp filemanager.cpp circle2d.cpp
WEBUI_SOURCES += line2d.cpp point2d.cpp sensorhandler.cpp zone.cpp bbengine.cpp
WEBUI_SOURCES += bblua.cpp rectangle.cpp stage.cpp cliprinthandler.cpp
WEBUI_SOURCES += zipper.cpp tarzipper.cpp bbrunner.cpp replaybuilder.cpp
WEBUI_SOURCES += relativebasedir.cpp relativerespath.cpp
WEBUI_SOURCES += ./luajit/src/libluajit.a
##############################################################################


##############################################################################
# osx: Sources and flags for building GUI on Mac OS X / Cocoa
OSX_EXTRA_SOURCES =  osxbasedir.mm osxcfg.m linuxrespath.cpp
OSX_EXTRA_SOURCES += ./luajit/src/libluajit.a
ifneq ($(LIBARCHIVE_PATH), )
  OSX_EXTRA_SOURCES += ${LIBARCHIVE_PATH}/.libs/libarchive.a
endif
OSX_EXTRA_SOURCES += /usr/lib/libz.dylib /usr/lib/libiconv.dylib

OSX_CFLAGS = -I./luajit/src -I./stlsoft-1.9.116/include
OSX_LDFLAGS = 

ifneq ($(LIBARCHIVE_PATH), )
  OSX_CFLAGS += -I${LIBARCHIVE_PATH}/libarchive
else
  OSX_LDFLAGS += -larchive
endif

ifneq ($(SFML_HEADER_PATH), )
  OSX_CFLAGS +=  -I${SFML_HEADER_PATH}
  OSX_LDFLAGS += -L./sfml-lib
endif

ifneq ($(WXWIDGETS_PATH), )
  OSX_CFLAGS +=  `${WXWIDGETS_PATH}/wx-config --cflags`
  OSX_LDFLAGS += `${WXWIDGETS_PATH}/wx-config --libs`
else
  OSX_CFLAGS +=  -I/usr/lib/x86_64-linux-gnu/wx/include/gtk2-unicode-3.0 -I/usr/include/wx-3.0 -D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__ -pthread
  OSX_LDFLAGS += -L/usr/lib/x86_64-linux-gnu -pthread   -lwx_gtk2u_xrc-3.0 -lwx_gtk2u_html-3.0 -lwx_gtk2u_qa-3.0 -lwx_gtk2u_adv-3.0 -lwx_gtk2u_core-3.0 -lwx_baseu_xml-3.0 -lwx_baseu_net-3.0 -lwx_baseu-3.0 
endif

OSX_LDFLAGS += -lsfml-graphics -lsfml-window -lsfml-system
OSX_LDFLAGS += -ldl -pagezero_size 10000 -image_base 100000000 -stdlib=libc++
##############################################################################


##############################################################################
# osxcli: Sources and flags for building CLI on Mac OS X / Cocoa
OSXCLI_EXTRA_SOURCES = relativebasedir.cpp relativerespath.cpp
OSXCLI_EXTRA_SOURCES += ./luajit/src/libluajit.a
OSXCLI_CFLAGS =  -I./luajit/src -I./stlsoft-1.9.116/include
OSXCLI_LDFLAGS =

ifneq ($(LIBARCHIVE_PATH), )
  OSXCLI_EXTRA_SOURCES += ${LIBARCHIVE_PATH}/.libs/libarchive.a
endif
OSXCLI_EXTRA_SOURCES += /usr/lib/libz.dylib /usr/lib/libiconv.dylib

ifneq ($(LIBARCHIVE_PATH), )
  OSXCLI_CFLAGS += -I${LIBARCHIVE_PATH}/libarchive
else
  OSXCLI_LDFLAGS += -larchive
endif

ifneq ($(SFML_HEADER_PATH), )
  OSXCLI_CFLAGS +=  -I${SFML_HEADER_PATH}
  OSXCLI_LDFLAGS += -L${SFML_LIB_PATH}
endif

OSXCLI_LDFLAGS += -lsfml-graphics -lsfml-window -lsfml-system -ldl
OSXCLI_LDFLAGS += -pagezero_size 10000 -image_base 100000000 -stdlib=libc++
##############################################################################


##############################################################################
# linux: Sources and flags for building GUI on Linux / GTK
LINUX_EXTRA_SOURCES =  linuxbasedir.cpp linuxrespath.cpp linuxcfg.cpp
LINUX_EXTRA_SOURCES += ./luajit/src/libluajit.a
ifneq ($(LIBARCHIVE_PATH), )
  LINUX_EXTRA_SOURCES += ${LIBARCHIVE_PATH}/.libs/libarchive.a
endif

LINUX_CFLAGS =  -I./luajit/src -I./stlsoft-1.9.116/include
LINUX_CFLAGS += -Wno-deprecated-declarations
LINUX_LDFLAGS =

ifneq ($(LIBARCHIVE_PATH), )
  LINUX_CFLAGS += -I${LIBARCHIVE_PATH}/libarchive
else
  LINUX_LDFLAGS += -larchive
endif

ifneq ($(SFML_HEADER_PATH), )
  LINUX_CFLAGS +=  -I${SFML_HEADER_PATH}
  LINUX_LDFLAGS += -L${SFML_LIB_PATH}
  LINUX_LDFLAGS += -Wl,-rpath,$(DESTDIR)$(libdir)/berrybots,--enable-new-dtags
endif

ifneq ($(WXWIDGETS_PATH), )
  LINUX_CFLAGS +=  `${WXWIDGETS_PATH}/wx-config --cflags`
  LINUX_LDFLAGS += `${WXWIDGETS_PATH}/wx-config --libs`
else
  LINUX_CFLAGS +=  -I/usr/lib/x86_64-linux-gnu/wx/include/gtk2-unicode-3.0 -I/usr/include/wx-3.0 -D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__ -pthread
  LINUX_LDFLAGS += -L/usr/lib/x86_64-linux-gnu -pthread   -lwx_gtk2u_xrc-3.0 -lwx_gtk2u_html-3.0 -lwx_gtk2u_qa-3.0 -lwx_gtk2u_adv-3.0 -lwx_gtk2u_core-3.0 -lwx_baseu_xml-3.0 -lwx_baseu_net-3.0 -lwx_baseu-3.0 
endif

LINUX_LDFLAGS += -lsfml-graphics -lsfml-window -lsfml-system -lz -ldl -lpthread
##############################################################################


##############################################################################
# linuxcli: Sources and flags for building CLI on Linux / GTK
LINUXCLI_EXTRA_SOURCES = relativebasedir.cpp relativerespath.cpp
LINUXCLI_EXTRA_SOURCES += ./luajit/src/libluajit.a
LINUXCLI_CFLAGS =  -I./luajit/src -I./stlsoft-1.9.116/include
LINUXCLI_CFLAGS += -Wno-deprecated-declarations
LINUXCLI_LDFLAGS =

ifneq ($(LIBARCHIVE_PATH), )
  LINUXCLI_EXTRA_SOURCES += ${LIBARCHIVE_PATH}/.libs/libarchive.a
endif

ifneq ($(LIBARCHIVE_PATH), )
  LINUXCLI_CFLAGS += -I${LIBARCHIVE_PATH}/libarchive
else
  LINUXCLI_LDFLAGS += -larchive
endif

ifneq ($(SFML_HEADER_PATH), )
  LINUXCLI_CFLAGS +=  -I${SFML_HEADER_PATH}
  LINUXCLI_LDFLAGS += -L${SFML_LIB_PATH}
endif

LINUXCLI_LDFLAGS += -lsfml-graphics -lsfml-window -lsfml-system -lz -ldl
LINUXCLI_LDFLAGS += -lpthread
##############################################################################


##############################################################################
# webui: Flags for building headless / save to replay on any platform.
WEBUI_CFLAGS =  -I./luajit/src -I./stlsoft-1.9.116/include
WEBUI_LDFLAGS = -ldl -lpthread

ifeq ($(.DEFAULT_GOAL), osx)
  WEBUI_SOURCES += /usr/lib/libz.dylib /usr/lib/libiconv.dylib
  WEBUI_LDFLAGS += -pagezero_size 10000 -image_base 100000000
endif
##############################################################################


##############################################################################
# Build targets
GUIPLATS = osx linux
CLIPLATS = rpi osxcli linuxcli

none:
	@echo "Please do                       "
	@echo "  make PLATFORM                 "
	@echo "where PLATFORM is one of these: "
	@echo "  GUI: $(GUIPLATS)     "
	@echo "  CLI: $(CLIPLATS)     "

MAKE_LUAJIT=cd luajit; $(MAKE); cd ..
CLEAN_LUAJIT=cd luajit; $(MAKE) clean; cd ..
MAKE_SFML=cd SFML-2.3.2; mkdir build; cd build; cmake -DCMAKE_SKIP_RPATH=ON ..; make; cd ../..
CLEAN_SFML=rm -rf SFML-2.3.2/build
MAKE_WX=cd wxWidgets-3.0.2; ./configure --disable-shared; $(MAKE); cd ..
CLEAN_WX=cd wxWidgets-3.0.2; make clean; cd ..
MAKE_LIBARCHIVE=cd libarchive-3.1.2; ./configure; $(MAKE); cd ..
CLEAN_LIBARCHIVE=cd libarchive-3.1.2; make clean; cd ..

rpi:
	$(MAKE_LUAJIT)
	$(CC) ${RPI_SOURCES} ${RPI_CFLAGS} ${RPI_LDFLAGS} -o berrybots
	@echo "==== Successfully built BerryBots $(VERSION) ===="
	@echo "==== Launch BerryBots with: ./berrybots"

osx:
	$(MAKE_LUAJIT)
ifeq ($(SFML_HEADER_PATH), )
	$(CC) ${SOURCES} ${OSX_EXTRA_SOURCES} ${OSX_CFLAGS} ${OSX_LDFLAGS} -o bbgui
	cp scripts/bb_gui.sh berrybots
else
	./scripts/osx_copy_fix_dylib_paths.sh "${SFML_LIB_PATH}" "sfml-lib"
	$(CC) ${SOURCES} ${OSX_EXTRA_SOURCES} ${OSX_CFLAGS} ${OSX_LDFLAGS} -o bbgui
	cp scripts/bb_gui_ldpath.sh berrybots
endif
	chmod 755 berrybots
	@echo "==== Successfully built BerryBots $(VERSION) ===="
	@echo "==== Launch BerryBots with: ./berrybots"

osxcli:
	$(MAKE_LUAJIT)
ifeq ($(SFML_HEADER_PATH), )
	$(CC) ${CLI_SOURCES} ${OSXCLI_EXTRA_SOURCES} ${OSXCLI_CFLAGS} ${OSXCLI_LDFLAGS} -o berrybots
else
	./scripts/osx_copy_fix_dylib_paths.sh "${SFML_LIB_PATH}" "sfml-lib"
	$(CC) ${CLI_SOURCES} ${OSXCLI_EXTRA_SOURCES} ${OSXCLI_CFLAGS} ${OSXCLI_LDFLAGS} -o bbmain
	cp scripts/bb_osx.sh berrybots
	chmod 755 berrybots
endif
	@echo "==== Successfully built BerryBots $(VERSION) ===="
	@echo "==== Launch BerryBots with: ./berrybots"

linux:
	$(MAKE_LUAJIT)
ifeq ($(SFML_HEADER_PATH), )
	$(CC) ${SOURCES} ${LINUX_EXTRA_SOURCES} ${LINUX_CFLAGS} ${LINUX_LDFLAGS} -o bbgui
	cp scripts/bb_gui.sh berrybots
else
  ifeq ($(LOCAL_SFML), 1)
	$(MAKE_SFML)
  endif
  ifeq ($(LOCAL_WX), 1)
	$(MAKE_WX)
  endif
  ifeq ($(LOCAL_LIBARCHIVE), 1)
	$(MAKE_LIBARCHIVE)
  endif
	$(CC) ${SOURCES} ${LINUX_EXTRA_SOURCES} ${LINUX_CFLAGS} ${LINUX_LDFLAGS} -o bbgui
	cp scripts/bb_gui_ldpath.sh berrybots
	rm -rf sfml-lib
	cp -r ${SFML_LIB_PATH} sfml-lib
endif
	chmod 755 berrybots
	@echo "==== Successfully built BerryBots $(VERSION) ===="
	@echo "==== Launch BerryBots with: ./berrybots"

linuxcli:
	$(MAKE_LUAJIT)
ifeq ($(SFML_HEADER_PATH), )
	$(CC) ${CLI_SOURCES} ${LINUXCLI_EXTRA_SOURCES} ${LINUXCLI_CFLAGS} ${LINUXCLI_LDFLAGS} -o berrybots
else
	$(CC) ${CLI_SOURCES} ${LINUXCLI_EXTRA_SOURCES} ${LINUXCLI_CFLAGS} ${LINUXCLI_LDFLAGS} -o bbmain
	rm -rf sfml-lib
	cp -r ${SFML_LIB_PATH} sfml-lib
	cp scripts/bb_linux.sh berrybots
	chmod 755 berrybots
endif
	@echo "==== Successfully built BerryBots $(VERSION) ===="
	@echo "==== Launch BerryBots with: ./berrybots"

webui:
	$(MAKE_LUAJIT)
	$(CC) ${WEBUI_SOURCES} ${WEBUI_CFLAGS} ${WEBUI_LDFLAGS} -o berrybots
	chmod 755 berrybots
	@echo "==== Successfully built BerryBots $(VERSION) ===="
	@echo "==== Launch BerryBots with: ./berrybots"

install:
ifeq ($(wildcard bbgui), ) 
	$(error Can only install BerryBots GUI targets.)
endif
	mkdir -p $(DESTDIR)$(bindir)
	cp bbgui $(DESTDIR)$(bindir)/berrybots
	mkdir -p $(DESTDIR)$(datadir)/berrybots
	cp LICENSE $(DESTDIR)$(datadir)/berrybots
	cp README $(DESTDIR)$(datadir)/berrybots
	cp -r resources $(DESTDIR)$(datadir)/berrybots
	cp -r bots $(DESTDIR)$(datadir)/berrybots
	cp -r stages $(DESTDIR)$(datadir)/berrybots
	cp -r runners $(DESTDIR)$(datadir)/berrybots
	cp -r apidoc $(DESTDIR)$(datadir)/berrybots
ifneq ($(SFML_HEADER_PATH), )
	mkdir -p $(DESTDIR)$(libdir)
	rm -rf $(DESTDIR)$(libdir)/berrybots
	cp -r sfml-lib $(DESTDIR)$(libdir)/berrybots
  ifeq ($(.DEFAULT_GOAL), osx)
	./scripts/osx_fix_dylib_paths.sh $(DESTDIR)$(bindir)/berrybots $(DESTDIR)$(libdir)/berrybots
  endif
endif
	mkdir -p $(DESTDIR)$(datarootdir)/pixmaps
	cp $(DESTDIR)$(datadir)/berrybots/resources/icon.iconset/icon_128x128.png $(DESTDIR)/$(datarootdir)/pixmaps/berrybots.png
	mkdir -p $(DESTDIR)$(datarootdir)/applications
	cp scripts/berrybots.desktop $(DESTDIR)$(datarootdir)/applications

uninstall:
	rm $(DESTDIR)$(bindir)/berrybots
	rm -rf $(DESTDIR)$(datadir)/berrybots
	rm -rf $(DESTDIR)$(libdir)/berrybots
	rm $(DESTDIR)$(datarootdir)/applications/berrybots.desktop
	rm $(DESTDIR)$(datarootdir)/pixmaps/berrybots.png

clean:
	$(CLEAN_LUAJIT)
ifeq ($(LOCAL_SFML), 1)
	$(CLEAN_SFML)
endif
ifeq ($(LOCAL_WX), 1)
	$(CLEAN_WX)
endif
ifeq ($(LOCAL_LIBARCHIVE), 1)
	$(CLEAN_LIBARCHIVE)
endif
	rm -rf *o sfml-lib bbgui berrybots.sh berrybots config.log config.status autom4te.cache

distclean: clean
	rm Makefile

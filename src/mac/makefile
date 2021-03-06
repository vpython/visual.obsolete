# In vpython-core2/src/mac execute sudo make setup to establish links
# Note however that apparently $(abspath makefile) and $(abspath ../../site-packages/visual)
#    are ignored on OSX 10.4, which led to some puzzled grief. It was necessary to create
#    a link in site-packages by hand.
# Be sure to specify what version of Python

PYTHON_VERSION = 3.1
PYTHON_FRAMEWORK = /Library/Frameworks/Python.framework/Versions/$(PYTHON_VERSION)/
SITE_PACKAGES = $(PYTHON_FRAMEWORK)lib/python$(PYTHON_VERSION)/site-packages/
TOP = ../vpython-core2/
DEPENDS = $(TOP)/dependencies/
SRC = $(TOP)/src/
BOOST = $(TOP)/dependencies/boost_files/mac_libs/
VPATH = $(SRC)mac $(SRC)core/util $(SRC)core $(SRC)python $(SRC)

# Use 4.0 compiler rather than 4.2 so as to build in a way compatible with OSX 10.5
CXX = /usr/bin/g++-4.0
CXXFLAGS = -fPIC -MMD -DNDEBUG \
	-mmacosx-version-min=10.4 \
	-arch i386 \
	-DPIC -O2 -g -finline-functions \
	-I$(TOP)include/mac -I$(TOP)include \
	-I$(PYTHON_FRAMEWORK)include/python$(PYTHON_VERSION) \
	-I$(SITE_PACKAGES)numpy/core/include \
	-I$(DEPENDS)boost_files -I$(DEPENDS)threadpool/include \
	-F/System/Library/Frameworks/AGL.framework \
	-F/System/Library/Frameworks/Carbon.framework \
	-F/System/Library/Frameworks/OpenGL.framework

OBJS = arrayprim.o arrow.o axial.o box.o cone.o cylinder.o display_kernel.o ellipsoid.o \
	frame.o label.o material.o mouse_manager.o mouseobject.o primitive.o pyramid.o \
	rectangular.o renderable.o ring.o sphere.o text.o \
	atomic_queue.o displaylist.o errors.o extent.o \
	gl_extensions.o gl_free.o icososphere.o light.o quadric.o \
	mac_display.o mac_font_renderer.o mac_random_device.o mac_rate.o mac_timer.o \
	render_manager.o rgba.o shader_program.o texture.o tmatrix.o vector.o\
	convex.o curve.o cvisualmodule.o extrusion.o faces.o \
	num_util.o numeric_texture.o points.o slice.o \
	wrap_arrayobjects.o wrap_display_kernel.o wrap_primitive.o \
	wrap_rgba.o wrap_vector.o

cvisual.so : $(OBJS)
	$(CXX) -mmacosx-version-min=10.4 \
     	-bundle -bundle_loader $(PYTHON_FRAMEWORK)bin/python$(PYTHON_VERSION) -o cvisual.so \
	$(OBJS) \
	-framework AGL -framework Carbon -framework OpenGL -framework Python \
	$(BOOST)libboost_python.a \
	$(BOOST)libboost_signals.a \
	$(BOOST)libboost_thread.a

install : cvisual.so
	cp cvisual.so ../vpython-core2/site-packages/vis
	sudo cp cvisual.so /Library/Frameworks/Python.framework/Versions/$(PYTHON_VERSION)/lib/python$(PYTHON_VERSION)/site-packages/vis
#	sudo cp cvisual.so /Library/Python/$(PYTHON_VERSION)/site-packages

clean :
	rm *.o
	rm *.d

# This command to be run from the vpython-core2/src/mac
setup : 
	mkdir -p ../../../vpython-core2/../mbuild
	ln -sfh $(abspath makefile) ../../../mbuild/makefile
	ln -sfh $(abspath ../../site-packages/visual) $(SITE_PACKAGES)visual

-include *.d

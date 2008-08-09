# Place this make file in a directory parallel to vpython-core2

TOP = ../vpython-core2
DEPENDS = $(TOP)/dependencies
SRC = $(TOP)/src
BOOST = $(TOP)/dependencies/boost_1_35_0/mac_lib
VPATH = $(SRC)/mac $(SRC)/core/util $(SRC)/core $(SRC)/python $(SRC)

CXX = /usr/bin/g++
CXXFLAGS = -fPIC -MMD -DNDEBUG \
	-Wno-long-double -DPIC -O2 -g -finline-functions \
	-I$(TOP)/include/mac -I$(TOP)/include \
	-I/System/Library/Frameworks/Python.framework/Versions/2.5/include/python2.5 \
	-I/Library/Python/2.5/site-packages/numpy/core/include \
	-I$(DEPENDS)/boost_1_35_0 -I$(DEPENDS)/threadpool/include \
	-F/System/Library/Frameworks/AGL.framework \
	-F/System/Library/Frameworks/Carbon.framework \
	-F/System/Library/Frameworks/OpenGL.framework

OBJS = arrow.o axial.o box.o cone.o cylinder.o display_kernel.o ellipsoid.o \
	frame.o label.o material.o mouse_manager.o mouseobject.o primitive.o pyramid.o \
	rectangular.o renderable.o ring.o sphere.o text.o \
	atomic_queue.o clipping_plane.o displaylist.o errors.o extent.o \
	gl_extensions.o gl_free.o icososphere.o lighting.o quadric.o \
	mac_display.o mac_font_renderer.o mac_random_device.o mac_rate.o mac_timer.o \
	render_manager.o rgba.o shader_program.o texture.o tmatrix.o vector.o\
	convex.o curve.o cvisualmodule.o faces.o \
	num_util.o numeric_texture.o points.o scalar_array.o slice.o \
	vector_array.o wrap_arrayobjects.o wrap_display_kernel.o wrap_primitive.o \
	wrap_rgba.o wrap_vector.o

cvisual.so : $(OBJS)
	$(CXX) -bundle -bundle_loader /usr/bin/python2.5 -o cvisual.so \
	$(OBJS) \
	-framework AGL -framework Carbon -framework OpenGL -framework Python \
	$(BOOST)/libboost_python-mt-s-1_35.a \
	$(BOOST)/libboost_signals-mt-s-1_35.a \
	$(BOOST)/libboost_thread-mt-s-1_35.a

install : cvisual.so
	sudo cp cvisual.so /Library/Python/2.5/site-packages

clean :
	rm *.o

-include *.d

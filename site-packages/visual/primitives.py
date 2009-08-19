# Code to complete the various primitive types
# Users should never import this module directly.  All of the public types and
# functions will be explicitly imported by __init__.py

from __future__ import division
import cvisual

from cvisual import vector
import crayola
color = crayola
from math import pi
from numpy import *

NCHORDS = 20.0 # number of chords in one coil of a helix

# Scenegraph management:
#   Renderable objects which become visible need to be added
#     to the scene graph using EITHER
#       - display.add_renderable() if not in a frame, OR
#       - frame.add_renderable()
#   Renderable objects which become invisible need to be removed using
#     the corresponding remove_renderable function.
#   If the display or frame of a visible object changes, it needs to
#     be removed and added.
# The py_renderable class encapsulates this logic, as well as
#   a fair amount of construction and attribute access common to
#   all renderables.

class py_renderable(object):
    def __init__(self, **keywords):
        _other = keywords.get("_other")
        if _other:
            del keywords["_other"]
            super(py_renderable,self).__init__(_other)
            self.__dict__ = dict(_other.__dict__)
            self.__display = _other.display
            self.__frame = _other.frame
            self.__visible = _other.visible
        else:
            super(py_renderable,self).__init__()
            self.__display = cvisual.display.get_selected()
            self.__frame = None
            self.__visible = True

        if keywords.has_key('display'):
            self.__display = keywords['display']
            del keywords['display']
        if keywords.has_key('visible'):
            self.__visible = keywords['visible']
            del keywords['visible']
        if keywords.has_key('frame'):
            self.__frame = keywords['frame']
            del keywords['frame']

        if not _other: self.init_defaults(keywords)

        self.process_init_args_from_keyword_dictionary( keywords )
 
        if self.__frame:
            if self.__frame.display != self.__display:
                raise ValueError, "Cannot initialize an object with a frame on a different display."

        self.check_init_invariants()

        if self.__visible:
            if self.__frame:
                self.__frame.add_renderable(self)
            elif self.__display:
                self.__display.add_renderable(self)

    def __copy__( self, **keywords):
        return self.__class__(_other=self, **keywords)

    def check_init_invariants(self):
        pass

    def set_display(self, display):
        "For internal use only. The setter for the display property."
        if display != self.__display:
        # Check that we aren't screwing up a frame.
            if self.__frame:
                raise ValueError, """Cannot change displays when within a
                    frame.  Make frame None, first."""
            if self.__display:
                self.__display.remove_renderable(self)
            self.__display = display
            self.__display.add_renderable(self)

    def get_display(self):
        "For internal use only.  The getter for the display property."
        return self.__display

    display = property( get_display, set_display)

    def get_frame(self):
        "For internal use only.  The getter for the frame property."
        return self.__frame

   # Overridden by the frame class below to add extra checks.
    def set_frame(self, frame):
        "For internal use only.  The setter for the frame property."
        if frame != self.__frame:
            if frame.display != self.__display:
                raise ValueError, "Cannot set to a frame on a different display."
            if frame and self.__frame:
                # Simply moving from one frame to another.
                self.__frame.remove_renderable(self)
                frame.add_renderable(self)
            elif frame and not self.__frame:
                # Moving into a reference frame when otherwise not in one.
                if self.__display:
                    self.__display.remove_renderable(self)
                frame.add_renderable(self)
            elif not frame and self.__frame:
                # Removing from a reference frame.
                self.__frame.remove_renderable(self)
                if self.__display:
                    self.__display.add_renderable(self)
            self.__frame = frame
            
    frame = property( get_frame, set_frame)

    def get_visible(self):
        "For internal use only.  The getter for the visible property."
        return self.__visible

    def set_visible(self, visible):
        "For internal use only.  The setter for the visible property."
        if visible and not self.__visible:
            if self.__frame:
                self.__frame.add_renderable(self)
            elif self.__display:
                self.__display.add_renderable(self)
        if not visible and self.__visible:
            if self.__frame:
                self.__frame.remove_renderable(self)
            elif self.__display:
                self.__display.remove_renderable(self)
        self.__visible = visible

    visible = property( get_visible, set_visible)

    def init_defaults(self, keywords):
        self.color = self.display.foreground
        if isinstance(self, cvisual.light):
            self.color = (1,1,1)
        elif 'material' not in keywords:
            self.material = self.display.material

    def process_init_args_from_keyword_dictionary( self, keywords ):
        if 'axis' in keywords: #< Should be set before 'length'
            self.axis = keywords['axis']
            del keywords['axis']

        # Assign all other properties
        for key, value in keywords.iteritems():
            setattr(self, key, value)

class py_renderable_uniform (py_renderable):
    def check_init_invariants(self):
        if not self.display.uniform:
            raise RuntimeError, "Do not create " + self.__class__.__name__ + " with nonuniform axes."      

class py_renderable_arrayobject (py_renderable):
   # Array objects are likely to need special handling in various places

    def get_red( self):
        return self.color[:,0]
    def get_green(self):
        return self.color[:,1]
    def get_blue(self):
        return self.color[:,2]
    def get_x(self):
        return self.pos[:,0]
    def get_y(self):
        return self.pos[:,1]
    def get_z(self):
        return self.pos[:,2]

    # Also none of them support opacity yet.   
    def set_opacity(self, opacity):
        raise RuntimeError, "Cannot yet specify opacity for curve, faces, convex, or points."
    opacity = property( None, set_opacity, None)

################################################################################
# Complete each type.

class distant_light (py_renderable, cvisual.distant_light):
    def set_pos(self, _): raise AttributeError("Attempt to set pos of a distant_light object.")
    pos = property(None,set_pos)

class local_light (py_renderable, cvisual.local_light):
    def set_direction(self, _): raise AttributeError("Attempt to set direction of a local_light object.")
    direction = property(None,set_direction)

class arrow (py_renderable_uniform, cvisual.arrow):
    pass

class cone (py_renderable_uniform, cvisual.cone):
    pass

class cylinder (py_renderable_uniform, cvisual.cylinder):
    pass

class sphere (py_renderable_uniform, cvisual.sphere):
    pass

class ring (py_renderable_uniform, cvisual.ring):
    pass

class box (py_renderable_uniform, cvisual.box):
    pass

class ellipsoid (py_renderable_uniform, cvisual.ellipsoid):
    pass

class pyramid (py_renderable_uniform, cvisual.pyramid ):
    pass

class label (py_renderable, cvisual.label):
    def init_defaults( self, keywords ):
        if not keywords.has_key('linecolor'):
            self.linecolor = self.display.foreground
        super(label, self).init_defaults( keywords )

class frame (py_renderable_uniform, cvisual.frame):
    def set_frame(self, frame):
        #Check to ensure that we are not establishing a cycle of reference frames.
        frame_iterator = frame
        while frame_iterator:
            if frame_iterator.frame is self:
                raise ValueError, "Attempted to create a cycle of reference frames."
            frame_iterator = frame_iterator.frame
        py_renderable_uniform.set_frame( self, frame)

class curve ( py_renderable_arrayobject, cvisual.curve ):
    pos = property( cvisual.curve.get_pos, cvisual.curve.set_pos, None)
    color = property( cvisual.curve.get_color, cvisual.curve.set_color, None)
    x = property( py_renderable_arrayobject.get_x, cvisual.curve.set_x, None)
    y = property( py_renderable_arrayobject.get_y, cvisual.curve.set_y, None)
    z = property( py_renderable_arrayobject.get_z, cvisual.curve.set_z, None)
    red = property( py_renderable_arrayobject.get_red, cvisual.curve.set_red, None)
    green = property( py_renderable_arrayobject.get_green, cvisual.curve.set_green, None)
    blue = property( py_renderable_arrayobject.get_blue, cvisual.curve.set_blue, None)

class points ( py_renderable_arrayobject, cvisual.points ):
    pos = property( cvisual.points.get_pos, cvisual.points.set_pos, None)
    color = property( cvisual.points.get_color, cvisual.points.set_color, None)
    x = property( py_renderable_arrayobject.get_x, cvisual.points.set_x, None)
    y = property( py_renderable_arrayobject.get_y, cvisual.points.set_y, None)
    z = property( py_renderable_arrayobject.get_z, cvisual.points.set_z, None)
    red = property( py_renderable_arrayobject.get_red, cvisual.points.set_red, None)
    green = property( py_renderable_arrayobject.get_green, cvisual.points.set_green, None)
    blue = property( py_renderable_arrayobject.get_blue, cvisual.points.set_blue, None)

class convex( py_renderable_arrayobject, py_renderable_uniform, cvisual.convex ):
    pos = property( cvisual.convex.get_pos, cvisual.convex.set_pos, None)

class faces( py_renderable_arrayobject, cvisual.faces ):
    pos = property( cvisual.faces.get_pos, cvisual.faces.set_pos, None)
    normal = property( cvisual.faces.get_normal, cvisual.faces.set_normal, None)
    color = property( cvisual.faces.get_color, cvisual.faces.set_color, None)

class helix(py_renderable):
    def __init__( self, _other=None, pos=vector(),
        x=None, y=None, z=None, red=None, green=None, blue=None,
        axis=vector(1,0,0), radius=1.0, length=None, up=vector(0,1,0),
        coils=5, thickness=None, color=color.white, **keywords):
        if keywords.has_key('display'):
            disp = keywords['display']
            del keywords['display']
        else:
            disp = cvisual.display.get_selected()
        if (not disp.uniform):
           raise RuntimeError, "Do not create helix with nonuniform axes."
        if keywords.has_key('frame'):
            fr = keywords['frame']
            del keywords['frame']
        else:
            fr = None
        self.process_init_args_from_keyword_dictionary( keywords )
        if x is not None:
            pos[0] = x
        if y is not None:
            pos[1] = y
        if z is not None:
            pos[2] = z
        if red is not None:
            color[0] = red
        if green is not None:
            color[1] = green
        if blue is not None:
            color[2] = blue
        axis = vector(axis)
        if length is None:
            length = axis.mag
        self.__length = length
        self.__axis = axis
        self.__radius = radius
        self.__up = up
        self.__coils = coils
        self.__thickness = radius/20.
        if thickness:
            self.__thickness = thickness
        self.__frame = frame(display=disp, frame=fr, pos=pos, axis=axis.norm(), up=up)
        self.helix = curve( frame = self.__frame, radius = self.__thickness/2.,
            color = color)
        self.create_pos()
      
    def create_pos(self):
        k = self.coils*(2*pi/self.__length)
        dx = (self.length/self.coils)/NCHORDS
        x_col = arange(0, self.__length+dx, dx)
        pos_data = zeros((len(x_col),3), float64)
        pos_data[:,0] = arange(0, self.__length+dx, dx)
        pos_data[:,1] = (self.radius) * sin(k*pos_data[:,0])
        pos_data[:,2] = (self.radius) * cos(k*pos_data[:,0])
        self.helix.pos = pos_data

    def set_pos(self, pos):
        self.__frame.pos = vector(pos)
    def get_pos(self):
        return self.__frame.pos

    def set_x(self, x):
        self.__frame.pos.x = x
    def get_x(self):
        return self.__frame.pos.x

    def set_y(self, y):
        self.__frame.pos.y = y
    def get_y(self):
        return self.__frame.pos.y

    def set_z(self, z):
        self.__frame.pos.z = z
    def get_z(self):
        return self.__frame.pos.z

    def set_color(self, color):
        self.helix.color = color
    def get_color(self):
        return self.helix.color

    def set_red(self, red):
        self.helix.red = red
    def get_red(self):
        return self.helix.red

    def set_green(self, green):
        self.helix.green = green
    def get_green(self):
        return self.helix.green

    def set_blue(self, blue):
        self.helix.blue = blue
    def get_blue(self):
        return self.helix.blue

    def set_radius(self, radius):
        scale = radius/self.__radius
        self.__radius = radius
        self.helix.y *= scale
        self.helix.z *= scale
    def get_radius(self):
        return self.__radius

    def set_axis(self, axis):
        axis = vector(axis)
        self.__axis = axis
        self.__frame.axis = axis.norm()
        self.set_length(axis.mag)
    def get_axis(self):
        return self.__axis

    def set_length(self, length):
        self.helix.x *= (length/self.__length)
        self.__length = length
        self.__frame.axis = self.__axis.norm()
        self.__axis = length*self.__frame.axis
    def get_length(self):
        return self.__length

    def set_coils(self, coils):
        if self.__coils == coils: return
        self.__coils = coils
        self.create_pos()
    def get_coils(self):
        return self.__coils

    def set_thickness(self, thickness):
        if self.__thickness == thickness: return
        self.__thickness = thickness
        self.helix.radius = thickness/2.
    def get_thickness(self):
        return self.__thickness

    def set_display(self, disp):
        self.helix.display = self.frame.display = disp
    def get_display(self):
        return self.helix.display

    def set_frame(self, fr):
        self.__frame.frame = fr
    def get_frame(self):
        return self.__frame.frame

    def set_up(self, up):
        self.__frame.up = up
    def get_up(self):
        return self.__frame.up

    pos = property( get_pos, set_pos, None)
    x = property( get_x, set_x, None)
    y = property( get_y, set_y, None)
    z = property( get_z, set_z, None)
    color = property( get_color, set_color, None)
    red = property( get_red, set_red, None)
    green = property( get_green, set_green, None)
    blue = property( get_blue, set_blue, None)
    axis = property( get_axis, set_axis, None)
    radius = property( get_radius, set_radius, None)
    coils = property( get_coils, set_coils, None)
    thickness = property( get_thickness, set_thickness, None)
    length = property( get_length, set_length, None)
    display = property( get_display, set_display, None)
    frame = property( get_frame, set_frame, None)
    up = property( get_up, set_up, None)

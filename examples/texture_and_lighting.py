from __future__ import division
from visual import *
# Bruce Sherwood, August 2006
# Demonstration of transparency (opacity), materials, and local lights
# in the new version 4 of VPython created by Jonathan Brandmeyer, reworked by David Scherer

scene.width = scene.height = 800
scene.forward = (-0.2,-0.2,-1)
width = 10 # of wood table
thick = 0.5 # thickness of wood
depth = 7 # of wood table
height = 2 # of side bars of table
xhit = height-thick # x distance of center of ball from side bar when it hits 
R = 2 # radius of ball
H = 10 # height of underside of ceiling above floor
L = 5 # length of pendulum to center of hanging lamp
# top of floor is at y=0 for convenience
floor = box(pos=(0,-thick/2,0), size=(width,thick,depth),
            shininess=0, color=color.orange, material=materials.wood)
left = box(pos=(-(width/2+thick/2),height/2-thick,0), size=(thick,height,depth),
            shininess=0, color=color.orange, material=materials.wood)
right = box(pos=(width/2+thick/2,height/2-thick,0), size=(thick,height,depth),
            shininess=0, color=color.orange, material=materials.wood)
back = box(pos=(0,height/2-thick,-(depth/2+thick/2)), size=(width+2*thick,height,thick),
            shininess=0, color=color.orange, material=materials.wood)
ceiling = box(pos=(0,H+thick/2,0), size=(width/10,thick,width/10), color=color.orange, material=materials.wood)
pendulum = frame(pos=(0,H,0), axis=(0,-1,0))
wire = curve(frame=pendulum, pos=[(0,0,0),(L,0,0)])
lamp = sphere(frame=pendulum, pos=(L,0,0), radius=0.03*L, color=1, material=materials.unlit)
sphere(pos=(0.1*width,R/4,0.45*depth), radius=R/4, color=color.red, material=materials.marble)
sphere(pos=(0.15*width,R/4,0.3*depth), radius=R/4, color=color.yellow, material=materials.marble)
sphere(pos=(0.15*width,R/4,-0.3*depth), radius=R/4, color=color.green, material=materials.marble)
sphere(pos=(0.1*width,R/4,-0.45*depth), radius=R/4, color=color.cyan, material=materials.marble)
scene.lights = []
scene.ambient = 0.25
l1 = light(pos=(6,2,4), color=0.3)
l2 = light(pos=(-10,2,4), color=0.2)
# Local lights currently don't move with a frame they're in.
# So we have to keep repositioning the lamplight.
lamplight = light(pos=(0,H-L,0), local=True, color=0.5)
scene.center = (0,0.4*H,0)
scene.range = 0.45*H

### Create a material for the ball
##M = N = 32
##t = zeros([M,N,3], float)
##colors = [color.blue, color.cyan, color.green, color.yellow]
##for i in range(0,M,3):
##     for j in range(0,N):
##         for ii in range(3):
##             print i, j, ii
##             c = colors[ii%3]
##             t[i+ii][j] = (c[0],c[1],c[2])
##balltex = materials.texture(data = t,
##                       channels = ["red","green","blue"],
##                       mapping = "rectangular")
##box(material=balltex)
balltex = materials.rough
ball = sphere(pos=(width/4,R,0), radius=R, up=(0,1,1),
              color=color.cyan, material=balltex, opacity=0.5)
v = vector(-0.5,0,0)
dt = 0.03
t = 0

while 1:
    rate(100)
    ball.pos += v*dt
    ball.rotate(axis=(0,0,1), angle=-v.x*dt/R)
    if not (-width/2+xhit < ball.x < width/2-xhit):
        v = -v
    angle = 0.02*cos(t)
    pendulum.rotate(axis=(1,0,0), angle=angle)
    r = lamplight.pos-pendulum.pos
    r = r.rotate(axis=(1,0,0), angle=angle)
    lamplight.pos = pendulum.pos+r
    t += dt

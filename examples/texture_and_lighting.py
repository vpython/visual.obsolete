from visual import *
from __future__ import division

# Bruce Sherwood, August 2006
# Demonstration of transparency (opacity), surface textures, and sophisticated lighting
#  in the new version 4 of VPython created by Jonathan Brandmeyer

scene.width = scene.height = 800
scene.forward = (-0.2,-0.2,-1)
# Read wood-like surface texture from a file.
# See vpython.org contributed programs for how this texture was created.
wood = texture(data=pickle.load(open('wood256lum.vpt', 'rU')))
width = 10 # of wood table
thick = 0.5 # thickness of wood
depth = 7 # of wood table
height = 2 # of side bars of table
xhit = height-thick # x distance of center of ball from side bar when it hits 
R = 2 # radius of ball
H = 10 # height of underside of ceiling above floor
L = 3.5 # length of pendulum to center of hanging lamp
# top of floor is at y=0 for convenience
floor = box(pos=(0,-thick/2,0), size=(width,thick,depth),
            shininess=0, color=color.orange, texture=wood)
left = box(pos=(-(width/2+thick/2),height/2-thick,0), size=(thick,height,depth),
            shininess=0, color=color.orange, texture=wood)
right = box(pos=(width/2+thick/2,height/2-thick,0), size=(thick,height,depth),
            shininess=0, color=color.orange, texture=wood)
back = box(pos=(0,height/2-thick,-(depth/2+thick/2)), size=(width+2*thick,height,thick), color=color.orange, texture=wood)
ceiling = box(pos=(0,H+thick/2,0), size=(width/10,thick,width/10), color=color.orange, texture=wood)
pendulum = frame(pos=(0,H,0), axis=(0,-1,0))
wire = curve(frame=pendulum, pos=[(0,0,0),(L,0,0)])
lamp = cylinder(frame=pendulum, pos=(0.9*L,0,0), axis=(0.2*L,0,0), radius=0.05*L, color=(0.7,0.7,0.7))
sphere(pos=(0.1*width,R/4,0.45*depth), radius=R/4, color=color.red)
sphere(pos=(0.15*width,R/4,0.3*depth), radius=R/4, color=color.yellow)
sphere(pos=(0.15*width,R/4,-0.3*depth), radius=R/4, color=color.green)
sphere(pos=(0.1*width,R/4,-0.45*depth), radius=R/4, color=color.cyan)
scene.lights = []
scene.ambient = 0.25
l1 = light(pos=(6,2,4), color=0.1)
l2 = light(pos=(-10,2,4), color=0.05)
# Note: It should be possible to put the spotlight in the pendulum frame, but
# currently there is a bug that spot_direction doesn't change when frame axis changes.
spot = light(pos=(0,H-L,0), local=True, color=0.5,
                  spot_direction=(0,-1,0), spot_cutoff=20)
scene.center = (0,0.4*H,0)
scene.range = 0.45*H

# Create a surface texture for the ball
M = N = 32
t = zeros([M,N,4], UnsignedInt8)
colors = [color.blue, color.cyan, color.green, color.yellow]
for i in range(0,M,4):
     for j in range(0,N):
         for ii in range(4):
                 c = colors[ii%4]
                 t[i+ii][j] = (255*c[0],255*c[1],255*c[2],128)
plaid = texture(data=t, type="rgba")
ball = sphere(pos=(width/4,R,0), radius=R, up=(0,1,1), texture=plaid)
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
    r = spot.pos-pendulum.pos
    r = r.rotate(axis=(1,0,0), angle=angle)
    spot.pos = pendulum.pos+r
    spot.spot_direction = norm(r)
    t += dt

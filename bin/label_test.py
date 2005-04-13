from visual import *

scene.title = "Basic label test"
scene.uniform = False
x = arrow(color=color.red)
tip = label( text="tip", space=.2, pos=(1,0,0))
tail = label( text="tail", xoffset=20, yoffset=10)
midp = label( text="a\nmidpoint", xoffset=10, yoffset=-20, pos=(.5,0,0))

print """You should see a single arrow with three labels attached to it.
        Each label is translucent.  The one at the tip should be centered at
        the tip.  The head and tail labels have a single line of text.  The
        midpoint label has two.\n"""

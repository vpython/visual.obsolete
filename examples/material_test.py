from visual import *
# Must eliminate glass and ice because they show the red box inside
show_sphere = True
scene.width = scene.height = 800
mats = [materials.wood, materials.rough, materials.marble, 
        materials.plastic, materials.earth, materials.diffuse,
        materials.emissive, materials.unshaded, # the old materials to here
        materials.metal,materials.chrome,
        materials.blazed, materials.silver, 
        materials.BlueMarble, materials.bricks]
names = ["wood", "rough", "marble",
         "plastic", "earth", "diffuse",
         "emissive", "unshaded",
         "metal", "chrome",
         "blazed", "silver",
         "BlueMarble", "bricks"]
D = 0.7 # size of box
R = .4 # radius of sphere
while True:
    for obj in scene.objects:
        obj.visible = False
        del obj
    scene.range = 2.2
    scene.fov = 0.01
    scene.autocenter = True
    label(pos=(2.5,-.3), text="Click or hit a key\nto toggle between\nspheres and boxes")
    index = 0
    for y in range(4):
        for x in range(4):
            if index >= 14: break
            if show_sphere:
                s = sphere(pos=(x,3-y,0), radius=R, material=mats[index])
                if names[index] == "bricks":
                    s.rotate(angle=pi/2, axis=(1,0,0))
            else:
                box(pos=(x,3-y,0), size=(D,D,D), material=mats[index])
            label(pos=(x,3-y-.5), text=names[index])
            index += 1
    while True:
        rate(30)
        if scene.mouse.events:
            m = scene.mouse.getevent()
            if m.click == 'left':
                show_sphere = (not show_sphere)
                break
        elif scene.kb.keys:
            scene.kb.getkey()
            show_sphere = (not show_sphere)
            break

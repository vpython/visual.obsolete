#ifndef VPYTHON_UTIL_QUADRIC_HPP
#define VPYTHON_UTIL_QUADRIC_HPP

struct GLUquadric;

// A thin wrapper around GLU quadric objects.
class quadric
{
 private:
	GLUquadric* q;
 
 public:
	enum drawing_style
	{ 
		POINT,
		LINE,
		FILL,
		SILHOUETTE		
	};
	
	enum normal_style
	{
		NONE,
		FLAT,
		SMOOTH		
	};
	
	enum orientation
	{
		OUTSIDE,
		INSIDE
	};
	
	// Create a new quadric object with smooth normals, filled drawing style,
	// outside orientation, and no texture coordinates.
	quadric();
	// Free up any resources that GLU required for the object.
	~quadric();
	
	// Set the properties associated with this object.
	void set_draw_style( drawing_style);
	void set_normal_style( normal_style);
	void set_orientation( orientation);
	void do_textures( bool);
	
	// Draw a sphere centered at the origin, with the pole pointing along the
	// y axis.
	void render_sphere( double radius, int slices, int stacks);
	
	// Draw a cylinder with these properties.  The cylinder's base is centered
	// at the origin, pointing along the +x axis.  Only the outer curve
	// of the cylinder is rendered, not the ends.
	void render_cylinder( double base_radius, double top_radius, double height,
		int slices, int stacks);
	
	// Draw a cylinder with constant radius, as above.
	void render_cylinder( double radius, double height, int slices, int stacks);
	
	// Draw a flat disk with these properties.  The disk is centered on the
	// origin, on the yz plane.
	void render_disk( double radius, int slices, int rings);
};

#endif // !defined VPYTHON_UTIL_QUADRIC_HPP

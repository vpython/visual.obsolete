#ifndef VPYTHON_CONVEX_HPP
#define VPYTHON_CONVEX_HPP

#include "renderable.hpp"
#include "util/sorted_model.hpp"

#include <vector>

class convex : public renderable
{
 private:
	std::vector<vector> pos;
	
	struct face : triangle
	{
		double d;
		inline face( const vector& v1, const vector& v2, const vector& v3)
			: triangle( v1, v2, v3), d( normal.dot(corner[0]))
		{
		}
		
		inline bool visible_from( const vector& p)
		{ return normal.dot(p) > d; }
	};
	
	struct edge
	{
		vector v[2];
		inline edge( vector a, vector b)
		{ v[0]=a; v[1]=b; }

		inline bool
		operator==( const edge& b) const
		{
			// There are two cases where a pair of edges are equal, the first is
			// occurs when the endpoints are both the same, while the other occurs 
			// when the edge have the same endpoints in opposite directions.  
			// Since the first case never happens when we construct the hull, we
			// only test for the second case here.
			return (v[0] == b.v[1] && v[1] == b.v[0]);
		}
	};
	
	
	struct jitter_table
	{
		enum { mask = 1023 };
		enum { count = mask+1 };
		double v[count];

		jitter_table()
		{
			for(int i=0; i<count; i++)
				v[i] = (static_cast<double>(rand()) / RAND_MAX - 0.5) * 2 * 1e-6;
		}
	}; 
	static jitter_table jitter;  // Use default construction for initialization.
	
	long last_checksum;
	long checksum() const;
	bool degenerate() const;
	
	// Hull construction routines.
	void recalc();
	void add_point( int, vector);
	std::vector<face> hull;
	
 public:
	convex();
	void append( vector pos);
	void set_color( const rgba&);
	
 protected:
	virtual void gl_render( const view&);
	virtual vector get_center() const;
	virtual void gl_pick_render( const view&);
	virtual void grow_extent( extent&);
};

#endif // !defined VPYTHON_CONVEX_HPP

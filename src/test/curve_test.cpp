#include "render_surface.hpp"
#include "arrow.hpp"
#include "curve.hpp"

int 
realmain( std::vector<std::string>&)
{
	basic_app main_window( "Curve test");
	vector v_gain(0, 0.02, 0);
	vector v_0( 2, 0, 0);
	double delta_angle = .1;
	
	shared_ptr<curve> thin( new curve());
	shared_ptr<curve> thick( new curve());
	thick->set_radius(0.05);
	
	tmatrix rotator = rotation( delta_angle, vector( 0, 1, 0));
	for (int i = 0; i < 300; ++i) {
		v_0 = rotator * v_0;
		thin->append( v_0 + v_gain*i, rgba(0,1,0));
		thick->append( v_0 - v_gain*i, rgba(0,0,1));
	}
	
	shared_ptr<arrow> x( new arrow());
	x->set_color( rgba( 1, 0, 0));
	
	main_window.scene.add_renderable( x);
	main_window.scene.add_renderable( thin);
	main_window.scene.add_renderable( thick);
	
	main_window.run();
	return 0;
}

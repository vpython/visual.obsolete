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
	for (int i = 0; i < 258; ++i) {
		v_0 = rotator * v_0;
		thin->append( v_0 + v_gain*i, rgb(0,1,0));
		thick->append( v_0 - v_gain*i, rgb(0,0,1));
#if 0 // Debugging code.
		if (i == 0) {
			shared_ptr<arrow> tmp(new arrow());
			tmp->set_pos( vector(0,0,0));
			tmp->set_axis( v_0+v_gain*i - vector(0,0,0));
			main_window.scene.add_renderable( tmp);
		}
		if (i >= 256) {
			shared_ptr<arrow> tmp(new arrow());
			tmp->set_pos( vector(0,5,0));
			tmp->set_axis( v_0+v_gain*i - vector(0,5,0));
			main_window.scene.add_renderable( tmp);
		}
#endif
	}
	
	shared_ptr<curve> box( new curve());
	box->set_radius( 0.1);
	box->append( vector( 2,-2), rgb(1, 0, 1));
	box->append( vector( 2, 2));
	box->append( vector(-2, 2));
	box->append( vector(-2,-2));
	box->append( vector(2,-2) );
	
	shared_ptr<arrow> x( new arrow());
	x->set_color( rgba( 1, 0, 0));
	
	main_window.scene.add_renderable( x);
	main_window.scene.add_renderable( thin);
	main_window.scene.add_renderable( thick);
	main_window.scene.add_renderable( box);
	
	main_window.run();
	return 0;
}

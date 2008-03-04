#include "material.hpp"

namespace cvisual {

void 
material::set_textures( std::vector< boost::shared_ptr< texture > > tex ) {
	textures = tex;
}

std::vector< boost::shared_ptr< texture > > 
material::get_textures() {
	return textures;
}

void 
material::set_shader( const std::string& source ) {
	if (source.size())
		shader.reset( new shader_program( source ) );
	else
		shader.reset( NULL );
}

std::string
material::get_shader() {
	if (shader)
		return shader->get_source();
	else
		return std::string();
}

void 
material::set_shininess( double s ) {
	shininess = clamp( 0.0, s, 1.0);
}

double material::get_shininess() {
	return shininess;
}

apply_material::apply_material( const view& v, material* m, tmatrix& model_material ) 
 : v(v), sp( v, m ? m->shader.get() : NULL )
{
	if (!m || !sp.ok()) return;
	char texa[] = "tex0";
	for(int t=0; t<m->textures.size(); t++) {
		if (t && v.glext.ARB_multitexture)
			v.glext.glActiveTexture(GL_TEXTURE0 + t);
		m->textures[t]->gl_activate(v);
		
		if (m->shader && v.glext.ARB_shader_objects) {
			texa[3] = '0'+t;
			v.glext.glUniform1i( m->shader->get_uniform_location( v, texa ), t );
		}
		if (!v.glext.ARB_multitexture) break;
	}
	
	// For compatibility, set the texture unit back
	if (m->textures.size() > 1 && v.glext.ARB_multitexture)
		v.glext.glActiveTexture(GL_TEXTURE0);

	int loc;
	if ( (loc = m->shader->get_uniform_location( v, "camera_world" )) >= 0 ) {
		float matrix[16];
		for(int i=0; i<16; i++)
			matrix[i] = v.camera_world.matrix_addr()[i];
		v.glext.glUniformMatrix4fv( loc, 1, false, matrix );
	}
	
	if ( (loc = m->shader->get_uniform_location( v, "model_material" )) >= 0 ) {
		float matrix[16];
		for(int i=0; i<16; i++)
			matrix[i] = model_material.matrix_addr()[i];
		v.glext.glUniformMatrix4fv( loc, 1, false, matrix );
	}
}

apply_material::~apply_material() {
}

} // namespace cvisual
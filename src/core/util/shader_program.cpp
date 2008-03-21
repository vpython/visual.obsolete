#include "util/shader_program.hpp"
#include "util/errors.hpp"
#include <boost/bind.hpp>

namespace cvisual {

shader_program::shader_program( const std::string& source )
 : source(source), program(-1)
{
}

shader_program::~shader_program() {
	if (program > 0)
		on_gl_free.free( boost::bind( &shader_program::gl_free, glDeleteObjectARB, program ) );
}

int shader_program::get_uniform_location( const view& v, const char* name ) {
	// xxx change interface to cache the uniforms we actually want and avoid string comparisons
	if (program <= 0 || !v.glext.ARB_shader_objects) throw std::runtime_error("get_uniform_location called on an unrealized shader program.");
	int& cache = uniforms[ name ];
	if (cache == 0)
		cache = 2 + v.glext.glGetUniformLocationARB( program, name );
	return cache - 2;
}

void shader_program::realize( const view& v ) {
	if (program != -1) return;
	
	if ( !v.glext.ARB_shader_objects )
		return;
	
	program = v.glext.glCreateProgramObjectARB();
	check_gl_error();
	
	compile( v, GL_VERTEX_SHADER_ARB, getSection("vertex") );
	compile( v, GL_FRAGMENT_SHADER_ARB, getSection("fragment") );

	v.glext.glLinkProgramARB( program );

	// Check if linking succeeded
	int link_ok = 0;
	v.glext.glGetObjectParameterivARB( program, GL_OBJECT_LINK_STATUS_ARB, &link_ok );
	
	if ( !link_ok ) {
		// Some drivers (incorrectly?) set the GL error in glLinkProgramARB() in this situation
		clear_gl_error();

		std::string infoLog;
		
		int length = 0;
		v.glext.glGetObjectParameterivARB( program, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length );
		boost::scoped_array<char> temp( new char[length+2] );
		v.glext.glGetInfoLogARB( program, length+1, &length, &temp[0] );
		infoLog.append( &temp[0], length );
		
		// xxx A way to report infoLog to the program?
		fprintf(stderr, "VPython: errors in shader program:\n%s\n", infoLog.c_str());
		
		// Get rid of the program, since it can't be used without generating GL errors.  We set
		//   program to 0 instead of -1 so that binding it will revert to the fixed function pipeline,
		//   and realize() won't be called again.
		v.glext.glDeleteObjectARB( program );
		program = 0;
	} else {
		check_gl_error();

		// xxx It's probably not technically legal to call glext functions from on_gl_free callbacks,
		// since they might run in a different context, even though the program _handle_ is shared.  Plus
		// this is kind of ugly.
		glDeleteObjectARB = v.glext.glDeleteObjectARB;
		on_gl_free.connect( boost::bind( &shader_program::gl_free, v.glext.glDeleteObjectARB, program ) );
	}
}

void shader_program::compile( const view& v, int type, const std::string& source ) {
	int shader = v.glext.glCreateShaderObjectARB( type );
	const char* str = source.c_str(); 
	int len = source.size();
	v.glext.glShaderSourceARB( shader, 1, &str, &len );
	v.glext.glCompileShaderARB( shader );
	v.glext.glAttachObjectARB( program, shader );
	v.glext.glDeleteObjectARB( shader );
}

std::string shader_program::getSection( const std::string& name ) {
	/* Extract section beginning with \n[name]\n and ending with \n[
	 e.g.
		[vertex]
		void main() {}
		[fragment]
		void main() {}
	*/

	std::string section;
	std::string header = "\n[" + name + "]\n";
	std::string source = "\n" + this->source;
	
	int p = 0;
	while ( (p = source.find( header, p )) != source.npos ) {
		p += header.size();
		int end = source.find( "\n[", p );
		if (end == source.npos) end = source.size();
		
		section += source.substr( p, end-p );
		p = end;
	}
	
	return section;
}

void
shader_program::gl_free( PFNGLDELETEOBJECTARBPROC glDeleteObjectARB, int program )
{
	glDeleteObjectARB(program);
}

use_shader_program::use_shader_program( const view& v, shader_program& program )
 : v(v)
{
	init(&program);
}

use_shader_program::use_shader_program( const view& v, shader_program* program )
 : v(v)
{
	init(program);
}

void use_shader_program::init(shader_program* program) {
	m_ok = false;
	if (!program) {
		oldProgram = -1;
		return;
	}
	
	if (!v.glext.ARB_shader_objects) return;
	
	program->realize(v);

	// xxx For now, nested shader invocations aren't supported.
	//oldProgram = v.glext.glGetHandleARB( GL_PROGRAM_OBJECT_ARB );
	oldProgram = 0;

	v.glext.glUseProgramObjectARB( program->program );
	check_gl_error();
	
	m_ok = (program->program != 0);
}

use_shader_program::~use_shader_program() {
	if (oldProgram<0 || !v.glext.ARB_shader_objects) return;
	v.glext.glUseProgramObjectARB( oldProgram );
}

} // namespace cvisual
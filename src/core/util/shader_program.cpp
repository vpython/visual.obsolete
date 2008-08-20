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

void shader_program::set_uniform_matrix( const view& v, int loc, const tmatrix& in ) {
	float matrix[16];
	const double* in_p = in.matrix_addr();
	for(int i=0; i<16; i++)
		matrix[i] = (float)in_p[i];
	v.glext.glUniformMatrix4fvARB( loc, 1, false, matrix );
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
	GLint link_ok = 0;
	v.glext.glGetObjectParameterivARB( program, GL_OBJECT_LINK_STATUS_ARB, &link_ok );
	
	if ( !link_ok ) {
		// Some drivers (incorrectly?) set the GL error in glLinkProgramARB() in this situation
		printf("!linkok\n");
		clear_gl_error();

		std::string infoLog;
		
		GLint length = 0;
		v.glext.glGetObjectParameterivARB( program, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length );
		boost::scoped_array<char> temp( new char[length+2] );
		v.glext.glGetInfoLogARB( program, length+1, &length, &temp[0] );
		infoLog.append( &temp[0], length );
		
		// xxx A way to report infoLog to the program?
		write_stderr( "VPython WARNING: errors in shader program:\n" + infoLog + "\n");
		
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

#ifdef __APPLE__
		v.glext.glUseProgramObjectARB( program );
		//GLint gpuVertexProcessing, gpuFragmentProcessing;
		GLint gpuVertexProcessing, gpuFragmentProcessing; // OS X 10.4 wants a long
		CGLGetParameter(CGLGetCurrentContext(), kCGLCPGPUVertexProcessing, &gpuVertexProcessing);
		// gpuVertexProcessing=1 on MacBook Pro (GeForce); gpuVertexProcessing=0 on MacBook (no graphics)
		if (!gpuVertexProcessing) program = 0;
		
		/*
		CGLGetParameter(CGLGetCurrentContext(), kCGLCPGPUFragmentProcessing, &gpuFragmentProcessing);
		printf("vertex %d, fragment %d\n", gpuVertexProcessing, gpuFragmentProcessing);
		
		CGLRendererInfoObj rend;
		//GLint nrend, value;
		long nrend, value;
		CGLQueryRendererInfo ( 255, &rend, &nrend);
		printf("nrend = %d\n", nrend);
		
		for (int n = 0; n < nrend; n++) {
			CGLDescribeRenderer(rend, n, kCGLRPAccelerated, &value);
			printf("%d accelerated = %d\n", n, value);
			CGLDescribeRenderer(rend, n, kCGLRPRendererID, &value);
			printf("%d RendererID = %x\n", n, value);
		}

		printf("destroy=%d\n", CGLDestroyRendererInfo(rend));
		
		/* 
		MacBook (no graphics)
		(ID=x24000, kCGLRendererIntel900ID)
		(ID=x22604, some kind of GeForce - all GeForce cards start with 22)
		(ID=x20400, kCGLRendererGenericFloatID)
		vertex 0, fragment 1
		nrend = 2
		0 accelerated = 1
		0 RendererID = 24000
		1 accelerated = 0
		1 RendererID = 20400
		destroy=0
		VPython ***CRITICAL ERROR***: ../vpython-core2/src/core/display_kernel.cpp:900: render_scene: OpenGL error: ../vpython-core2/src/core/box.cpp:68 invalid operation, aborting.

		Assertion failed: (!pthread_mutex_destroy(&internal_mutex)), function ~condition_variable_any, file ../vpython-core2/dependencies/boost_1_35_0/boost/thread/pthread/condition_variable.hpp, line 86.
		Abort trap
		
		MacBook Pro (GeForce)
		vertex 1, fragment 1
		nrend = 2
		0 accelerated = 1
		0 RendererID = 22604
		1 accelerated = 0
		1 RendererID = 20400
		destroy=0
		*/
		
		v.glext.glUseProgramObjectARB( 0 );
#endif
	}
}

void shader_program::compile( const view& v, int type, const std::string& source ) {
	int shader = v.glext.glCreateShaderObjectARB( type );
	const char* str = source.c_str(); 
	GLint len = source.size();
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
#include <vector>
#include <string>

int realmain( std::vector<std::string>& args);

int
main( int argc, char** argv)
{
	if (argc < 1)
		argc = 1;
	std::vector<std::string> args( argc-1);
	for (int i = 1; i < argc; ++i) {
		args[i-1] = std::string(argv[i]);
	}
	return realmain(args);
}

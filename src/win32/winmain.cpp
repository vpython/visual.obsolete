#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

int realmain( vector<string>& args);

int
WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int)
{
	istringstream cmd(lpCmdLine);
	vector<string> args;
	istream_iterator<string> begin(cmd);
	if (begin != istream_iterator<string>())
		++begin;
	
	copy( istream_iterator<string>(cmd), istream_iterator<string>(),
		back_inserter(args));
	return realmain(args);
}

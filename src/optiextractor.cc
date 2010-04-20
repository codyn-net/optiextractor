#include "application.hh"

using namespace std;
using namespace optiextractor;

int main(int argc, char **argv)
{
	Application application(argc, argv);
	application.Run(argc, argv);

	return 0;
}

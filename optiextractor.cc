#include "Application/application.hh"

#include <iostream>
using namespace std;

int main(int argc, char **argv)
{
	Gtk::Main loop(argc, argv);

	if (argc < 2)
	{
		cerr << "Please provide database to open" << endl;
		return 1;
	}

	Application application;
	
	application.open(argv[1]);
	Gtk::Main::run(application.window());
	
	return 0;
}

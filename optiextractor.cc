#include "Application/application.hh"

#include <iostream>
using namespace std;

int main(int argc, char **argv)
{
	Gtk::Main loop(argc, argv);

	Application application;
	
	if (argc > 1)
	{
		application.open(argv[1]);
	}
	
	Gtk::Main::run(application.window());
	
	return 0;
}

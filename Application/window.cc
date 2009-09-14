#include "application.ih"

Gtk::Window &Application::window()
{
	if (!d_window)
	{
		initializeUI();
	}
	
	return *d_window;
}

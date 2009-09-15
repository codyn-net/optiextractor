#include "application.ih"

void Application::openResponse(int response)
{
	if (response == Gtk::RESPONSE_OK)
	{
		open(d_openDialog->get_filename());
	}

	delete d_openDialog;
	d_openDialog = 0;
}

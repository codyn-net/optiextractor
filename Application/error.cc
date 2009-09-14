#include "application.ih"

void Application::error(string const &error)
{
	destroyDialog(0);

	d_dialog = new Gtk::MessageDialog(window(), error, true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
	d_dialog->show();
	
	d_dialog->signal_response().connect(sigc::mem_fun(*this, &Application::destroyDialog));
}

#include "application.ih"

void Application::error(string const &error, string const &secondary)
{
	destroyDialog(0);

	d_dialog = new Gtk::MessageDialog(window(), error, true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);

	d_dialog->set_transient_for(window());
	d_dialog->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
	
	if (secondary != "")
	{
		d_dialog->set_secondary_text(secondary, true);
	}

	window().show();
	d_dialog->show();

	d_dialog->signal_response().connect(sigc::mem_fun(*this, &Application::destroyDialog));
}

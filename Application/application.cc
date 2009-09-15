#include "application.ih"

Application::Application()
:
	d_window(0),
	d_dialog(0),
	d_openDialog(0)
{
	d_runner.onState.add(*this, &Application::runnerState);
	d_runner.onResponse.add(*this, &Application::runnerResponse);
	
	window().set_default_icon_name(Gtk::Stock::EXECUTE.id);
}

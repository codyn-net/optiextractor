#include "application.ih"

void Application::runnerState(bool running)
{
	Glib::RefPtr<Gtk::Button> button = get<Gtk::Button>("button_execute");
	
	if (!running)
	{
		button->set_label(Gtk::Stock::EXECUTE.id);
		handleRunnerStopped();
	}
	else
	{
		button->set_label(Gtk::Stock::STOP.id);
		handleRunnerStarted();
	}
}

#include "application.ih"

void Application::handleRunnerStopped()
{
	if (d_lastResponse.status() == Response::Failed &&
	    d_lastResponse.failure().type() == Response::Failure::Disconnected)
	{
		return;
	}

	// Show response dialog
	Gtk::Dialog *dialog;
	
	d_builder->get_widget("dialog_response", dialog);
	
	Glib::RefPtr<Gtk::Label> label = get<Gtk::Label>("label_response_info");
	Glib::RefPtr<Gtk::TextView> tv = get<Gtk::TextView>("text_view_response_info");
	Glib::RefPtr<Gtk::ScrolledWindow> sw = get<Gtk::ScrolledWindow>("scrolled_window_response_info");

	dialog->set_transient_for(window());

	if (d_lastResponse.status() == Response::Failed)
	{
		string message;
		
		switch (d_lastResponse.failure().type())
		{
			case Response::Failure::Timeout:
				message = "A timeout occurred";
			break;
			case Response::Failure::DispatcherNotFound:
				message = "The dispatcher process could not be found";
			break;
			case Response::Failure::NoResponse:
				message = "There was no response from the dispatcher";
			break;
			case Response::Failure::Dispatcher:
				message = "An error occurred in the dispatcher";
			break;
			default:
				message = "An unknown error occurred";
			break;
		}
		
		label->set_text(message);
		string error = d_runner.error();
		
		if (d_lastResponse.failure().message() != "")
		{
			if (error != "")
			{
				error += "\n\n";
			}
			
			error += d_lastResponse.failure().message();
		}
		
		if (error != "")
		{
			tv->get_buffer()->set_text(error);
			sw->show();
		}
		else
		{
			sw->hide();
		}
	}
	else
	{
		stringstream s;
		s << "Solution ran successfully: ";
		
		for (size_t i = 0; i < d_lastResponse.fitness_size(); ++i)
		{
			Response::Fitness const &fitness = d_lastResponse.fitness(i);
			
			if (i != 0)
			{
				s << ", ";
			}
			
			s << fitness.name() << " = " << fitness.value() << endl;
		}
		
		label->set_text(s.str());
		sw->hide();
	}

	dialog->run();
	dialog->hide();
}

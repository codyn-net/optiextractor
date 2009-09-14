#include "application.ih"

void Application::runSolution()
{
	d_runner.cancel();

	d_lastResponse = Response();
	d_lastResponse.set_status(Response::Failed);
	d_lastResponse.mutable_failure()->set_type(Response::Failure::NoResponse);
		
	// Create dispatch request
	Request::Dispatch dispatch;
	
	dispatch.set_job("");
	dispatch.set_optimizer(d_database.query("SELECT `value` FROM settings WHERE `name` = 'optimizer'").get<string>(0));
	
	Glib::RefPtr<Gtk::ListStore> store = get<Gtk::ListStore>("list_store_solutions");
	Gtk::TreeModel::iterator iter;
	
	/* Set parameters */
	for (iter = store->children().begin(); iter != store->children().end(); ++iter)
	{
		Gtk::TreeRow r = *iter;
		string name;
		string value;
		
		r.get_value(0, name);
		r.get_value(0, value);
		
		/* Check boundaries */
		std::map<std::string, std::string>::iterator fnd = d_parameterMap.find(name);
		double range[2] = {0, 0};
		
		if (fnd != d_parameterMap.end())
		{
			Row row = d_database.query("SELECT `min`, `max` FROM boundaries WHERE `name` = '" + fnd->second + "'");
			
			if (!row.done())
			{
				range[0] = row.get<double>(0);
				range[1] = row.get<double>(1);
			}
		}

		Request::Dispatch::Parameter *parameter;
		parameter = dispatch.add_parameters();

		parameter->set_name(name);
		parameter->set_value(String(value));
		parameter->set_min(range[0]);
		parameter->set_max(range[1]);
	}
	
	/* Set dispatcher settings */
	store = get<Gtk::ListStore>("list_store_dispatcher");
	std::map<std::string, std::string> settings;
	
	for (iter = store->children().begin(); iter != store->children().end(); ++iter)
	{
		Gtk::TreeRow r = *iter;
		string name;
		string value;
		
		r.get_value(0, name);
		r.get_value(1, value);
		
		if (name != "dispatcher")
		{
			settings[name] = value;
		}
	}
	
	store = get<Gtk::ListStore>("list_store_override");
	
	for (iter = store->children().begin(); iter != store->children().end(); ++iter)
	{
		Gtk::TreeRow r = *iter;
		string name;
		string value;
		
		r.get_value(0, name);
		r.get_value(1, value);
		
		settings[name] = value;
	}
	
	for (std::map<string, string>::iterator it = settings.begin(); it != settings.end(); ++it)
	{
		Request::Dispatch::KeyValue *setting;
		setting = dispatch.add_settings();
	
		setting->set_key(it->first);
		setting->set_value(it->second);
	}
	
	Row row = d_database.query("SELECT `value` FROM dispatcher WHERE `name` = 'fitness'");
	
	if (row.done())
	{
		dispatch.set_fitness("");
	}
	else
	{
		dispatch.set_fitness(row.get<string>(0));
	}
	
	string dispatcher = resolveDispatcher();
	
	if (dispatcher == "")
	{
		/* Ask user for custom path */
		Gtk::Dialog *dialog;
		
		d_builder->get_widget("dialog_dispatcher", dialog);
		
		Glib::RefPtr<Gtk::FileChooser> chooser = get<Gtk::FileChooser>("file_chooser_button_dispatcher");
		dialog->set_transient_for(window());
	
		if (d_overrideDispatcher != "")
		{
			chooser->set_filename(d_overrideDispatcher);
		}

		if (dialog->run() == Gtk::RESPONSE_OK)
		{
			dispatcher = chooser->get_filename();
			d_overrideDispatcher = dispatcher;
		}

		dialog->hide();
	}
	
	if (dispatcher != "")
	{
		if (!d_runner.run(dispatch, dispatcher))
		{
			error("Could not spawn dispatcher: " + dispatcher);
		}
	}
}

#include "application.ih"

void Application::clear()
{
	get<Gtk::Notebook>("notebook")->set_sensitive(false);

	get<Gtk::Range>("hscale_solution")->set_value(0);
	get<Gtk::Range>("hscale_iteration")->set_value(0);
	
	get<Gtk::ListStore>("list_store_settings")->clear();
	get<Gtk::ListStore>("list_store_dispatcher")->clear();
	get<Gtk::ListStore>("list_store_fitness")->clear();
	get<Gtk::ListStore>("list_store_parameters")->clear();
	get<Gtk::ListStore>("list_store_boundaries")->clear();
	get<Gtk::ListStore>("list_store_solutions")->clear();
	get<Gtk::ListStore>("list_store_override")->clear();
	get<Gtk::ListStore>("list_store_log")->clear();
	
	get<Gtk::Label>("label_summary_optimizer")->set_text("");
	get<Gtk::Label>("label_summary_time")->set_text("");
	get<Gtk::Label>("label_summary_best")->set_text("");
	
	get<Gtk::Label>("label_solution_fitness")->set_text("");
	get<Gtk::Label>("label_solution_iteration")->set_text("");
	get<Gtk::Label>("label_solution_solution")->set_text("");
	
	d_runner.cancel();

	d_scanned = false;
	d_dispatchers.clear();
	
	d_lastResponse = Response();
	d_lastResponse.mutable_failure()->set_type(Response::Failure::NoResponse);
	
	d_parameterMap.clear();
	
	d_overrideDispatcher = "";
}

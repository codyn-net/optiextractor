#include "application.ih"

void Application::initializeUI()
{
	d_builder = Glib::RefPtr<Gtk::Builder>(Gtk::Builder::create_from_file(DATADIR "/optiextractor/window.xml"));
	d_builder->get_widget("window", d_window);
	
	d_listStoreSettings = get<Gtk::ListStore>("list_store_settings");
	
	get<Gtk::Range>("hscale_solution")->signal_value_changed().connect(sigc::mem_fun(*this, &Application::solutionChanged));
	get<Gtk::Range>("hscale_iteration")->signal_value_changed().connect(sigc::mem_fun(*this, &Application::solutionChanged));
	
	get<Gtk::Button>("button_execute")->signal_clicked().connect(sigc::mem_fun(*this, &Application::executeClicked));
	
	get<Gtk::Button>("button_add_override")->signal_clicked().connect(sigc::mem_fun(*this, &Application::overrideAdd));
	get<Gtk::Button>("button_remove_override")->signal_clicked().connect(sigc::mem_fun(*this, &Application::overrideRemove));
	
	get<Gtk::CellRendererText>("cell_renderer_text_override_name")->signal_edited().connect(sigc::mem_fun(*this, &Application::overrideNameEdited));

	get<Gtk::CellRendererText>("cell_renderer_text_override_value")->signal_edited().connect(sigc::mem_fun(*this, &Application::overrideValueEdited));
			
	d_window->set_sensitive(false);
}

#include "application.ih"

void lala()
{
	cout << "lala" << endl;
}

void Application::initializeUI()
{
	d_builder = Glib::RefPtr<Gtk::Builder>(Gtk::Builder::create_from_file(DATADIR "/optiextractor/window.xml"));
	d_builder->get_widget("window", d_window);
	
	/* Create menu */
	d_uiManager = Gtk::UIManager::create();
	
	d_actionGroup = Gtk::ActionGroup::create();
	d_actionGroup->add(Gtk::Action::create("FileMenuAction", "_File"));
	d_actionGroup->add(Gtk::Action::create("FileOpenAction", Gtk::Stock::OPEN), 
	                   sigc::mem_fun(*this, &Application::onFileOpen));
	d_actionGroup->add(Gtk::Action::create("FileExportAction", "_Export"),
	                   sigc::mem_fun(*this, &Application::onFileExport));
	d_actionGroup->add(Gtk::Action::create("FileCloseAction", Gtk::Stock::CLOSE),
	                   sigc::mem_fun(*this, &Application::onFileClose));
	d_actionGroup->add(Gtk::Action::create("FileQuitAction", Gtk::Stock::QUIT),
	                   sigc::mem_fun(*this, &Application::onFileQuit));
	
	d_uiManager->insert_action_group(d_actionGroup);

	d_window->add_accel_group(d_uiManager->get_accel_group());

	Glib::ustring ui_info =
	"<ui>"
	"  <menubar name='MenuBar'>"
	"    <menu name='FileMenu' action='FileMenuAction'>"
	"      <menuitem name='FileOpen' action='FileOpenAction'/>"
	"      <menuitem name='FileExport' action='FileExportAction'/>"
	"      <menuitem name='FileClose' action='FileCloseAction'/>"
	"      <menuitem name='FileQuit' action='FileQuitAction'/>"
	"    </menu>"
	"  </menubar>"
	"</ui>";

	d_uiManager->add_ui_from_string(ui_info);
	
	Gtk::Widget *menu = d_uiManager->get_widget("/ui/MenuBar");
	get<Gtk::VBox>("vbox_main")->pack_start(*menu, Gtk::PACK_SHRINK);

	get<Gtk::Range>("hscale_solution")->signal_value_changed().connect(sigc::mem_fun(*this, &Application::solutionChanged));
	get<Gtk::Range>("hscale_iteration")->signal_value_changed().connect(sigc::mem_fun(*this, &Application::solutionChanged));
	
	get<Gtk::Button>("button_execute")->signal_clicked().connect(sigc::mem_fun(*this, &Application::executeClicked));
	
	get<Gtk::Button>("button_add_override")->signal_clicked().connect(sigc::mem_fun(*this, &Application::overrideAdd));
	get<Gtk::Button>("button_remove_override")->signal_clicked().connect(sigc::mem_fun(*this, &Application::overrideRemove));
	
	get<Gtk::CellRendererText>("cell_renderer_text_override_name")->signal_edited().connect(sigc::mem_fun(*this, &Application::overrideNameEdited));

	get<Gtk::CellRendererText>("cell_renderer_text_override_value")->signal_edited().connect(sigc::mem_fun(*this, &Application::overrideValueEdited));

	d_window->set_sensitive(false);
}

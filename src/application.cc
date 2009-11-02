#include "application.hh"
#include <db/db.hh>
#include <os/os.hh>
#include <base/base.hh>

#include <sys/types.h>
#include <dirent.h>

using namespace std;
using namespace db;
using namespace os;
using namespace base;
using namespace optimization::messages::task;

Application::Application()
:
	d_window(0),
	d_dialog(0),
	d_openDialog(0),
	d_scanned(false)
{
	d_runner.OnState.add(*this, &Application::RunnerState);
	d_runner.OnResponse.add(*this, &Application::RunnerResponse);
	
	Window().set_default_icon_name(Gtk::Stock::EXECUTE.id);
}

void Application::Clear()
{
	Get<Gtk::Notebook>("notebook")->set_sensitive(false);

	Get<Gtk::Range>("hscale_solution")->set_value(0);
	Get<Gtk::Range>("hscale_iteration")->set_value(0);
	
	Get<Gtk::ListStore>("list_store_settings")->clear();
	Get<Gtk::ListStore>("list_store_dispatcher")->clear();
	Get<Gtk::ListStore>("list_store_fitness")->clear();
	Get<Gtk::ListStore>("list_store_parameters")->clear();
	Get<Gtk::ListStore>("list_store_boundaries")->clear();
	Get<Gtk::ListStore>("list_store_solutions")->clear();
	Get<Gtk::ListStore>("list_store_override")->clear();
	Get<Gtk::ListStore>("list_store_log")->clear();
	
	Get<Gtk::Label>("label_summary_optimizer")->set_text("");
	Get<Gtk::Label>("label_summary_time")->set_text("");
	Get<Gtk::Label>("label_summary_best")->set_text("");
	
	Get<Gtk::Label>("label_solution_fitness")->set_text("");
	Get<Gtk::Label>("label_solution_iteration")->set_text("");
	Get<Gtk::Label>("label_solution_solution")->set_text("");
	
	d_runner.Cancel();

	d_scanned = false;
	d_dispatchers.clear();
	
	d_lastResponse = Response();
	d_lastResponse.mutable_failure()->set_type(Response::Failure::NoResponse);
	
	d_parameterMap.clear();
	
	d_overrideDispatcher = "";
}

void
Application::DestroyDialog(int response)
{
	if (d_dialog)
	{
		delete d_dialog;
	}
}

void
Application::Error(string const &error, string const &secondary)
{
	DestroyDialog(0);

	d_dialog = new Gtk::MessageDialog(Window(), error, true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);

	d_dialog->set_transient_for(Window());
	d_dialog->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
	
	if (secondary != "")
	{
		d_dialog->set_secondary_text(secondary, true);
	}

	Window().show();
	d_dialog->show();

	d_dialog->signal_response().connect(sigc::mem_fun(*this, &Application::DestroyDialog));
}

void
Application::ExecuteClicked()
{
	if (d_runner)
	{
		// Cancel it
		d_runner.Cancel();
	}
	else
	{
		RunSolution();
	}
}

void
Application::FillBoundaries()
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_boundaries");
	
	store->clear();
	
	Row row = d_database.query("SELECT `name`, `min`, `max` FROM boundaries ORDER BY `name`");
	
	if (row.done())
	{
		return;
	}
	
	while (row)
	{
		Gtk::TreeRow iter = *(store->append());
		
		iter.set_value(0, row.get<string>(0));
		iter.set_value(1, row.get<string>(1));
		iter.set_value(2, row.get<string>(2));
		
		row.next();
	}
}

void
Application::Fill()
{
	if (!d_database)
	{
		return;
	}

	Get<Gtk::Notebook>("notebook")->set_sensitive(true);
	
	Get<Gtk::Label>("label_summary_optimizer")->set_text(d_database.query("SELECT `value` FROM settings WHERE `name` = 'optimizer'").get<string>(0));

	/* Best fitness */
	Get<Gtk::Label>("label_summary_best")->set_text(d_database.query("SELECT MAX(`best_fitness`) FROM `iteration`").get<string>(0));
	
	time_t first = d_database.query("SELECT `time` FROM `iteration` WHERE `iteration` = 0").get<size_t>(0);
	time_t last = d_database.query("SELECT `time` FROM `iteration` ORDER BY `time` DESC LIMIT 1").get<size_t>(0);
	
	Get<Gtk::Label>("label_summary_time")->set_text(FormatDate(first) + "   to   "  + FormatDate(last));

	FillKeyValue("list_store_settings", "settings", "name", "value", "`name` <> 'optimizer'");
	FillKeyValue("list_store_fitness", "fitness_settings", "name", "value");
	FillKeyValue("list_store_dispatcher", "dispatcher", "name", "value");
	
	FillKeyValue("list_store_parameters", "parameters", "name", "boundary");
	
	Gtk::TreeModel::iterator iter;
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_parameters");\

	for (iter = store->children().begin(); iter != store->children().end(); ++iter)
	{
		Gtk::TreeRow r = *iter;
		string name;
		string value;
		
		r.get_value(0, name);
		r.get_value(1, value);
		
		d_parameterMap[name] = value;
	}
	
	FillBoundaries();
	FillLog();
	FillOverrides();
	
	size_t maxiteration = d_database.query("SELECT MAX(`iteration`) FROM `solution`").get<size_t>(0);
	size_t maxindex = d_database.query("SELECT MAX(`index`) FROM `solution`").get<size_t>(0);
	
	Get<Gtk::Range>("hscale_iteration")->set_range(0, maxiteration + 1);
	Get<Gtk::Range>("hscale_solution")->set_range(0, maxindex + 1);
	
	SolutionChanged();
}

void
Application::FillKeyValue(Glib::RefPtr<Gtk::ListStore> store, string const &table, string const &keyname, string const &valuename, string const &condition)
{
	store->clear();
	
	stringstream q;
	
	q << "SELECT `" << keyname << "`, `" << valuename << "` FROM `" << table << "`";
	
	if (condition != "")
	{
		q << " WHERE " << condition;
	}
	
	q << " ORDER BY `" << keyname << "`";
	
	Row row = d_database.query(q.str());
	
	if (row.done())
	{
		return;
	}
	
	while (row)
	{
		Gtk::TreeRow iter = *(store->append());
		
		iter.set_value(0, row.get<string>(0));
		
		try
		{
			iter.set_value(1, row.get<string>(1));
		}
		catch (exception &e)
		{
		}

		row.next();
	}
}

void
Application::FillKeyValue(string const &storeName, string const &table, string const &keyname, string const &valuename, string const &condition)
{
	FillKeyValue(Get<Gtk::ListStore>(storeName), table, keyname, valuename, condition);
}

void
Application::FillLog()
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_log");
	
	store->clear();
	Row row = d_database.query("SELECT * FROM log");
	
	if (row.done())
	{
		return;
	}
	
	while (row)
	{
		Gtk::TreeRow r = *(store->append());
		
		r->set_value(0, FormatDate(row.get<size_t>(0)));
		r->set_value(1, row.get<string>(1));
		r->set_value(2, row.get<string>(2));

		row.next();
	}
}

void Application::FillOverrides()
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_override");
	
	store->clear();
	Row row = d_database.query("SELECT `name`, `value` FROM optiextractor_overrides");
	
	if (row.done())
	{
		return;
	}
	
	while (row)
	{
		Gtk::TreeRow r = *(store->append());
		
		r->set_value(0, row.get<string>(0));
		r->set_value(1, row.get<string>(1));

		row.next();
	}
}

string
Application::FormatDate(size_t timestamp, std::string const &format) const
{
	char buffer[1024];
	time_t t = static_cast<time_t>(timestamp);

	struct tm *tt = localtime(&t);
	strftime(buffer, 1024, format.c_str(), tt);
	
	return string(buffer);
}

void Application::HandleRunnerStarted()
{
	// Don't really need to do anything...
}

void
Application::HandleRunnerStopped()
{
	if (d_lastResponse.status() == Response::Failed &&
	    d_lastResponse.failure().type() == Response::Failure::Disconnected)
	{
		return;
	}

	// Show response dialog
	Gtk::Dialog *dialog;
	
	d_builder->get_widget("dialog_response", dialog);
	
	Glib::RefPtr<Gtk::Label> label = Get<Gtk::Label>("label_response_info");
	Glib::RefPtr<Gtk::TextView> tv = Get<Gtk::TextView>("text_view_response_info");
	Glib::RefPtr<Gtk::ScrolledWindow> sw = Get<Gtk::ScrolledWindow>("scrolled_window_response_info");

	dialog->set_transient_for(Window());

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
		string error = d_runner.Error();
		
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

void
Application::InitializeUI()
{
	d_builder = Glib::RefPtr<Gtk::Builder>(Gtk::Builder::create_from_file(DATADIR "/optiextractor/window.xml"));
	d_builder->get_widget("window", d_window);
	
	/* Create menu */
	d_uiManager = Gtk::UIManager::create();
	
	d_actionGroup = Gtk::ActionGroup::create();
	d_actionGroup->add(Gtk::Action::create("FileMenuAction", "_File"));
	d_actionGroup->add(Gtk::Action::create("FileOpenAction", Gtk::Stock::OPEN), 
	                   sigc::mem_fun(*this, &Application::OnFileOpen));
	d_actionGroup->add(Gtk::Action::create("FileCloseAction", Gtk::Stock::CLOSE),
	                   sigc::mem_fun(*this, &Application::OnFileClose));
	d_actionGroup->add(Gtk::Action::create("FileQuitAction", Gtk::Stock::QUIT),
	                   sigc::mem_fun(*this, &Application::OnFileQuit));

	d_actionGroup->add(Gtk::Action::create("DatabaseMenuAction", "_Database"));
	d_actionGroup->add(Gtk::Action::create("DatabaseExportAction", "_Export"),
	                   sigc::mem_fun(*this, &Application::OnDatabaseExport));
	d_actionGroup->add(Gtk::Action::create("DatabaseOptimizeAction", "_Optimize"),
	                   sigc::mem_fun(*this, &Application::OnDatabaseOptimize));
	
	d_uiManager->insert_action_group(d_actionGroup);

	d_window->add_accel_group(d_uiManager->get_accel_group());

	Glib::ustring ui_info =
	"<ui>"
	"  <menubar name='MenuBar'>"
	"    <menu name='FileMenu' action='FileMenuAction'>"
	"      <menuitem name='FileOpen' action='FileOpenAction'/>"
	"      <separator/>"
	"      <menuitem name='FileClose' action='FileCloseAction'/>"
	"      <menuitem name='FileQuit' action='FileQuitAction'/>"
	"    </menu>"
	"    <menu name='DatabaseMenu' action='DatabaseMenuAction'>"
	"      <menuitem name='DatabaseExport' action='DatabaseExportAction'/>"
	"      <separator/>"
	"      <menuitem name='DatabaseOptimize' action='DatabaseOptimizeAction'/>"
	"    </menu>"
	"  </menubar>"
	"</ui>";

	d_uiManager->add_ui_from_string(ui_info);
	
	Gtk::Widget *menu = d_uiManager->get_widget("/ui/MenuBar");
	Get<Gtk::VBox>("vbox_main")->pack_start(*menu, Gtk::PACK_SHRINK);

	Get<Gtk::Range>("hscale_solution")->signal_value_changed().connect(sigc::mem_fun(*this, &Application::SolutionChanged));
	Get<Gtk::Range>("hscale_iteration")->signal_value_changed().connect(sigc::mem_fun(*this, &Application::SolutionChanged));
	
	Get<Gtk::Button>("button_execute")->signal_clicked().connect(sigc::mem_fun(*this, &Application::ExecuteClicked));
	
	Get<Gtk::Button>("button_add_override")->signal_clicked().connect(sigc::mem_fun(*this, &Application::OverrideAdd));
	Get<Gtk::Button>("button_remove_override")->signal_clicked().connect(sigc::mem_fun(*this, &Application::OverrideRemove));
	
	Get<Gtk::CellRendererText>("cell_renderer_text_override_name")->signal_edited().connect(sigc::mem_fun(*this, &Application::OverrideNameEdited));

	Get<Gtk::CellRendererText>("cell_renderer_text_override_value")->signal_edited().connect(sigc::mem_fun(*this, &Application::OverrideValueEdited));

	Clear();
}

void
Application::OnDatabaseExport()
{
	Gtk::FileChooserDialog *dialog;
	
	dialog = new Gtk::FileChooserDialog(Window(), 
	                                   "Export Optimization Database",
	                                    Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog->add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

	dialog->set_local_only(true);
	
	Gtk::FileFilter *filter = new Gtk::FileFilter();

	filter->set_name("Matlab data text files (*.txt)");
	filter->add_pattern("*.txt");
	dialog->add_filter(*filter);
	delete filter;
	
	filter = new Gtk::FileFilter();
	filter->set_name("All files (*)");
	filter->add_pattern("*");
	dialog->add_filter(*filter);
	delete filter;

	if (dialog->run() != Gtk::RESPONSE_OK)
	{
		delete dialog;
	
		return;
	}
	
	string filename = dialog->get_filename();
	delete dialog;

	ofstream fstr(filename.c_str(), ios::out);
	
	if (!fstr)
	{
		Error("Could not create file to export to");
		return;
	}
	
	size_t iterations = d_database.query("SELECT COUNT(*) FROM `solution` GROUP BY `index`").get<size_t>(0);
	size_t solutions = d_database.query("SELECT COUNT(*) FROM `solution`").get<size_t>(0);
	
	/* Collect fitness names */
	Row cols = d_database.query("PRAGMA table_info(`fitness`)");
	vector<string> fitnesses;
	vector<string> colnames;
	
	fitnesses.push_back("value");
	colnames.push_back("`fitness`.value");
	
	if (!cols.done())
	{
		while (cols)
		{
			String name = cols.get<string>(1);
			
			if (name.startsWith("_"))
			{
				fitnesses.push_back(name);
				colnames.push_back("`fitness`." + name);
			}
			
			cols.next();
		}
	}
	
	if (fitnesses.size() == 2)
	{
		fitnesses.erase(fitnesses.begin() + 1);
		colnames.erase(colnames.begin() + 1);
	}
	
	vector<string> boundaries;
	
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_boundaries");
	Gtk::TreeModel::iterator iter;

	for (iter = store->children().begin(); iter != store->children().end(); ++iter)
	{
		for (size_t i = 0; i < 3; ++i)
		{
			string value;
			(*iter)->get_value(i, value);
			boundaries.push_back(value);
		}
	}
	
	std::map<std::string, std::string>::iterator it;
	vector<string> parameters;
	
	for (it = d_parameterMap.begin(); it != d_parameterMap.end(); ++it)
	{
		parameters.push_back(it->first);
		parameters.push_back(it->second);
	}
	
	size_t totalnum = iterations * solutions;
	string names = String::join(colnames, ", ");
	
	Row row = d_database.query("SELECT solution.`iteration`, solution.`index`, `values`, `velocity`, `value_names`, " + names + " FROM `solution` LEFT JOIN `fitness` ON (fitness.iteration = solution.iteration AND fitness.`index` = solution.`index`) ORDER BY solution.`iteration`, solution.`index`");
	
	bool header = false;
	
	if (!row.done())
	{
		while (row)
		{
			if (!header)
			{
				size_t num = String(row.get<string>(2)).split(",").size();
				string nm = String::join(String(row.get<string>(4)).split(","), "\t");
				string fitnm = String::join(fitnesses, "\t");
				
				fstr << num << "\t" << iterations << "\t" << solutions << "\t" << nm << "\t" << fitnm << endl;
				fstr << String::join(boundaries, "\t") << endl;
				fstr << String::join(parameters, "\t") << endl;
				
				header = true;
			}
			
			fstr << row.get<size_t>(0) << "\t" << row.get<size_t>(1);
			
			for (size_t i = 5; i < 5 + fitnesses.size(); ++i)
			{
				fstr << row.get<string>(i);
			}
			
			fstr << String::join(String(row.get<string>(2)).split(","), "\t");
			fstr << String::join(String(row.get<string>(3)).split(","), "\t");
			fstr << endl;

			row.next();
		}
	}
	
	fstr.close();
}

void
Application::OnDatabaseOptimize()
{
	/* Create indices if necessary */
	d_database.begin();
	d_database.query("CREATE INDEX IF NOT EXISTS iteration_iteration ON iteration(`iteration`)");
	d_database.query("CREATE INDEX IF NOT EXISTS solution_index ON solution(`index`)");
	d_database.query("CREATE INDEX IF NOT EXISTS solution_iteration ON solution(`iteration`)");
	
	d_database.query("CREATE INDEX IF NOT EXISTS fitness_index ON fitness(`index`)");
	d_database.query("CREATE INDEX IF NOT EXISTS fitness_iteration ON fitness(`iteration`)");

	d_database.commit();
}

void
Application::OnFileClose()
{
	Clear();
}

void
Application::OnFileOpen()
{
	/* Open dialog */
	if (d_openDialog)
	{
		delete d_openDialog;
	}
	
	d_openDialog = new Gtk::FileChooserDialog(Window(), 
	                                          "Open Optimization Database",
	                                          Gtk::FILE_CHOOSER_ACTION_OPEN);

	d_openDialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	d_openDialog->add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	d_openDialog->set_local_only(true);

	d_openDialog->signal_response().connect(sigc::mem_fun(*this, &Application::OpenResponse));
	d_openDialog->show();
}

void
Application::OnFileQuit()
{
	Gtk::Main::quit();
}

void
Application::Open(string const &filename)
{
	Clear();
	
	if (!FileSystem::fileExists(filename))
	{
		Error("<b>Database file does not exist</b>", "The file '<i>" + filename + "</i>' does not exist. Please make sure you open a valid optimization results database.");
		return;
	}
	
	d_database = SQLite(filename);
	
	if (!d_database)
	{
		Error("<b>Database could not be opened</b>", "The file '<i>" + filename + "</i>' could not be opened. Please make sure the file is a valid optimization results database.");
		return;
	}

	// Do a quick test query
	Row row = d_database.query("PRAGMA table_info(settings)");

	if (!d_database.query("PRAGMA quick_check") || (!row || row.done()))
	{
		Error("<b>Database could not be opened</b>", "The file '<i>" + filename + "</i>' could not be opened. Please make sure the file is a valid optimization results database.");
		return;
	}
	
	// Make sure to create the optiextractor_override table to store overrides
	row = d_database.query("PRAGMA table_info(optiextractor_overrides)");

	if (!row || row.done())
	{
		d_database.query("CREATE TABLE `optiextractor_overrides` (`name` TEXT, `value` TEXT)");
	}
	
	Fill();
}

void
Application::OpenResponse(int response)
{
	if (response == Gtk::RESPONSE_OK)
	{
		Open(d_openDialog->get_filename());
	}

	delete d_openDialog;
	d_openDialog = 0;
}

void
Application::OverrideAdd()
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_override");
	Glib::RefPtr<Gtk::TreeView> tv = Get<Gtk::TreeView>("tree_view_override");
	
	Gtk::TreeIter iter = store->append();
	Gtk::TreeRow row = *iter;
	
	row.set_value(0, string("key"));
	row.set_value(1, string("value"));
	
	Gtk::TreePath path = store->get_path(iter);
	
	tv->scroll_to_cell(path, *tv->get_column(0), 0.5, 0.5);
	tv->set_cursor(path, *tv->get_column(0), true);
	
	d_database.query("DELETE FROM `optiextractor_overrides` WHERE `name` = 'key'");
	d_database.query("INSERT INTO `optiextractor_overrides` (`name`, `value`) VALUES ('key', 'value')");
}

void
Application::OverrideNameEdited(Glib::ustring const &path, Glib::ustring const &newtext)
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_override");

	Gtk::TreeModel::iterator iter = store->get_iter(path);
	Gtk::TreeRow row = *iter;
	
	string current;

	row->get_value(0, current);	
	row->set_value(0, newtext);
	
	d_database.query() << "UPDATE `optiextractor_overrides` SET `name` = '"
	                   << SQLite::serialize(newtext)
	                   << "' WHERE `name` = '"
	                   << SQLite::serialize(current)
	                   << "'"
	                   << SQLite::Query::End();
}

void
Application::OverrideRemove()
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_override");
	Glib::RefPtr<Gtk::TreeView> tv = Get<Gtk::TreeView>("tree_view_override");
	
	Gtk::TreeModel::iterator iter = tv->get_selection()->get_selected();
	
	if (!iter)
	{
		return;
	}
	
	string name;
	iter->get_value(0, name);
	
	d_database.query() << "DELETE FROM optiextractor_overrides WHERE `name` = '" 
	                   << SQLite::serialize(name) << "'"
	                   << SQLite::Query::End();
	
	iter = store->erase(iter);
		
	if (iter)
	{
		tv->get_selection()->select(iter);
	}
	else
	{
		size_t num = store->children().size();
		
		if (num != 0)
		{
			stringstream s;
			s << (num - 1);
			
			tv->get_selection()->select(Gtk::TreePath(s.str()));
		}
	}
}

void
Application::OverrideValueEdited(Glib::ustring const &path, Glib::ustring const &newtext)
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_override");

	Gtk::TreeModel::iterator iter = store->get_iter(path);
	Gtk::TreeRow row = *iter;
	
	string current;

	row->get_value(0, current);
	row->set_value(1, newtext);
	
	d_database.query() << "UPDATE `optiextractor_overrides` SET `value` = '"
	                   << SQLite::serialize(newtext)
	                   << "' WHERE `name` = '"
	                   << SQLite::serialize(current) << "'"
	                   << SQLite::Query::End();
}

string
Application::ResolveDispatcher()
{
	// First try to find the dispatcher from the database
	Row row = d_database.query("SELECT `value` FROM `dispatcher` WHERE `name` = 'name'");
	
	if (row.done())
	{
		// No dispatcher found
		return "";
	}
	
	string dispatcher = row.get<string>(0);

	/* Try to locate dispatcher */
	if (FileSystem::isAbsolute(dispatcher))
	{
		return FileSystem::fileExists(dispatcher) ? dispatcher : "";
	}
	else
	{
		Scan();

		std::map<std::string, std::string>::iterator fnd = d_dispatchers.find(dispatcher);
		dispatcher = (fnd != d_dispatchers.end()) ? fnd->second : "";
	}
	
	return dispatcher;
}

void
Application::RunnerResponse(Response const &response)
{
	d_lastResponse = response;
}

void
Application::RunnerState(bool running)
{
	Glib::RefPtr<Gtk::Button> button = Get<Gtk::Button>("button_execute");
	
	if (!running)
	{
		button->set_label(Gtk::Stock::EXECUTE.id);
		HandleRunnerStopped();
	}
	else
	{
		button->set_label(Gtk::Stock::STOP.id);
		HandleRunnerStarted();
	}
}

void
Application::RunSolution()
{
	d_runner.Cancel();

	d_lastResponse = Response();
	d_lastResponse.set_status(Response::Failed);
	d_lastResponse.mutable_failure()->set_type(Response::Failure::NoResponse);
		
	// Create dispatch request
	Task::Description description;
	
	description.set_job("");
	description.set_optimizer(d_database.query("SELECT `value` FROM settings WHERE `name` = 'optimizer'").get<string>(0));
	
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_solutions");
	Gtk::TreeModel::iterator iter;
	
	// Set parameters
	for (iter = store->children().begin(); iter != store->children().end(); ++iter)
	{
		Gtk::TreeRow r = *iter;
		string name;
		string value;
		
		r.get_value(0, name);
		r.get_value(1, value);
		
		/* Check boundaries */
		std::map<std::string, std::string>::iterator fnd = d_parameterMap.find(name);
		double range[2] = {0, 0};
		
		if (fnd != d_parameterMap.end())
		{
			Row row = d_database.query("SELECT `min`, `max` FROM boundaries WHERE `name` = '" + SQLite::serialize(fnd->second) + "'");
			
			if (!row.done())
			{
				range[0] = row.get<double>(0);
				range[1] = row.get<double>(1);
			}
		}

		Task::Description::Parameter *parameter;
		parameter = description.add_parameters();

		parameter->set_name(name);
		parameter->set_value(String(value));
		parameter->set_min(range[0]);
		parameter->set_max(range[1]);
	}
	
	/* Set dispatcher settings */
	store = Get<Gtk::ListStore>("list_store_dispatcher");
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
	
	store = Get<Gtk::ListStore>("list_store_override");
	
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
		Task::Description::KeyValue *setting;
		setting = description.add_settings();
	
		setting->set_key(it->first);
		setting->set_value(it->second);
	}
	
	string dispatcher = ResolveDispatcher();
	
	if (dispatcher == "")
	{
		/* Ask user for custom path */
		Gtk::Dialog *dialog;
		
		d_builder->get_widget("dialog_dispatcher", dialog);
		
		Glib::RefPtr<Gtk::FileChooser> chooser = Get<Gtk::FileChooser>("file_chooser_button_dispatcher");
		dialog->set_transient_for(Window());
	
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
		if (!d_runner.Run(description, dispatcher))
		{
			Error("Could not spawn dispatcher: " + dispatcher);
		}
	}
}

void
Application::Scan()
{
	if (d_scanned)
	{
		return;
	}
	
	vector<string> paths = Environment::path("OPTIMIZATION_DISPATCHERS_PATH");
	
	for (vector<string>::iterator iter = paths.begin(); iter != paths.end(); ++iter)
	{
		Scan(*iter);
	}
	
	string path;

	if (FileSystem::realpath(PREFIXDIR "/libexec/liboptimization-dispatchers-1.0", path))
	{
		Scan(path);
	}
	
	d_scanned = true;
}

void
Application::Scan(string const &directory)
{
	if (!FileSystem::directoryExists(directory))
	{
		return;
	}
	
	DIR *d = opendir(directory.c_str());
	
	if (!d)
	{
		return;
	}
	
	struct dirent *dent;
	
	while ((dent = readdir(d)))
	{
		String s(Glib::build_filename(directory, dent->d_name));
		struct stat buf;

		if (stat(s.c_str(), &buf) != 0)
		{
			continue;
		}

		if (!S_ISREG(buf.st_mode) && !S_ISLNK(buf.st_mode))
		{
			continue;
		}
		
		if (!(buf.st_mode & S_IXUSR))
		{
			continue;
		}

		d_dispatchers[dent->d_name] = s;
	}
	
	closedir(d);
}

void
Application::SolutionChanged()
{
	Get<Gtk::Label>("label_solution_fitness")->set_text("");
	Get<Gtk::Label>("label_solution_iteration")->set_text("");
	Get<Gtk::Label>("label_solution_solution")->set_text("");
	
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_solutions");
	store->clear();
	
	if (!Get<Gtk::Notebook>("notebook")->sensitive())
	{
		return;
	}
	
	size_t iteration = Get<Gtk::Range>("hscale_iteration")->get_value();
	size_t solution = Get<Gtk::Range>("hscale_solution")->get_value();
	
	stringstream q;
	
	q << "SELECT `iteration`, `index`, `fitness`, `values`, `value_names` FROM solution";
	
	if (solution == 0 && iteration != 0)
	{
		q << " WHERE `iteration` = " << (iteration - 1);
	}
	else if (iteration == 0 && solution != 0)
	{
		q << " WHERE `index` = " << (solution - 1);
	}
	else if (iteration != 0 && solution != 0)
	{
		q << " WHERE `iteration` = " << (iteration - 1) << " AND `index` = " << (solution - 1);
	}
	
	q << " ORDER BY `fitness` DESC LIMIT 1";
	
	Row row = d_database.query(q.str());
	
	if (row.done())
	{
		return;
	}
	
	{
		stringstream s;
		s << (row.get<size_t>(0) + 1);
		Get<Gtk::Label>("label_solution_iteration")->set_text(s.str());
	}

	{
		stringstream s;
		s << (row.get<size_t>(1) + 1);
		Get<Gtk::Label>("label_solution_solution")->set_text(s.str());
	}
	
	Get<Gtk::Label>("label_solution_fitness")->set_text(row.get<string>(2));
	
	vector<string> values = String(row.get<string>(3)).split(",");
	vector<string> names = String(row.get<string>(4)).split(",");
	
	for (size_t i = 0; i < values.size(); ++i)
	{
		Gtk::TreeRow row = *(store->append());
		
		row.set_value(0, string(String(names[i]).strip()));
		row.set_value(1, string(String(values[i]).strip()));
	}
}

Gtk::Window &
Application::Window()
{
	if (!d_window)
	{
		InitializeUI();
	}
	
	return *d_window;
}

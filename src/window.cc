#include "window.hh"

#include <jessevdk/os/os.hh>

using namespace std;
using namespace jessevdk::db;
using namespace jessevdk::os;
using namespace jessevdk::base;
using namespace optimization::messages::task;
using namespace optiextractor;

Window::Window()
:
	d_dialog(0),
	d_openDialog(0)
{
	d_runner.OnState.Add(*this, &Window::RunnerState);
	d_runner.OnResponse.Add(*this, &Window::RunnerResponse);

	d_window->set_default_icon_name(Gtk::Stock::EXECUTE.id);

	InitializeUI();
}

void
Window::Clear()
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
	Get<Gtk::ListStore>("list_store_solution_fitness")->clear();
	Get<Gtk::ListStore>("list_store_override")->clear();
	Get<Gtk::ListStore>("list_store_log")->clear();

	Get<Gtk::Label>("label_summary_optimizer")->set_text("");
	Get<Gtk::Label>("label_summary_time")->set_text("");
	Get<Gtk::Label>("label_summary_best")->set_text("");

	Get<Gtk::Label>("label_solution_iteration")->set_text("");
	Get<Gtk::Label>("label_solution_solution")->set_text("");

	d_runner.Cancel();

	d_lastResponse = Response();
	d_lastResponse.mutable_failure()->set_type(Response::Failure::NoResponse);

	d_parameterMap.clear();

	d_overrideDispatcher = "";
}

void
Window::DestroyDialog(int response)
{
	if (d_dialog)
	{
		delete d_dialog;
	}
}

void
Window::Error(string const &error, string const &secondary)
{
	DestroyDialog(0);

	d_dialog = new Gtk::MessageDialog(*d_window, error, true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);

	d_dialog->set_transient_for(*d_window);
	d_dialog->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

	if (secondary != "")
	{
		d_dialog->set_secondary_text(secondary, true);
	}

	d_window->show();
	d_dialog->show();

	d_dialog->signal_response().connect(sigc::mem_fun(*this, &Window::DestroyDialog));
}

void
Window::ExecuteClicked()
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
Window::FillBoundaries()
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_boundaries");

	store->clear();

	sqlite::Row row = d_database("SELECT `name`, `min`, `max` FROM boundaries ORDER BY `name`");

	if (row.Done())
	{
		return;
	}

	while (row)
	{
		Gtk::TreeRow iter = *(store->append());

		iter.set_value(0, row.Get<string>(0));
		iter.set_value(1, row.Get<string>(1));
		iter.set_value(2, row.Get<string>(2));

		row.Next();
	}
}

void
Window::Fill()
{
	if (!d_database)
	{
		return;
	}

	Get<Gtk::Notebook>("notebook")->set_sensitive(true);

	Get<Gtk::Label>("label_summary_optimizer")->set_text(d_database("SELECT `optimizer` FROM job").Get<string>(0));

	/* Best fitness */
	sqlite::Row row = d_database("SELECT 1 FROM iteration LIMIT 1");
	if (row && !row.Done())
	{
		Get<Gtk::Label>("label_summary_best")->set_text(d_database("SELECT MAX(`best_fitness`) FROM `iteration`").Get<string>(0));

		time_t first = d_database("SELECT `time` FROM `iteration` WHERE `iteration` = 0").Get<size_t>(0);
		time_t last = d_database("SELECT `time` FROM `iteration` ORDER BY `time` DESC LIMIT 1").Get<size_t>(0);

		Get<Gtk::Label>("label_summary_time")->set_text(FormatDate(first) + "   to   "  + FormatDate(last));
	}
	else
	{
		Get<Gtk::Label>("label_summary_best")->set_text("");
		Get<Gtk::Label>("label_summary_time")->set_text("");
	}

	FillKeyValue("list_store_settings", "settings", "name", "value");
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

	size_t maxiteration = d_database("SELECT MAX(`iteration`) FROM `solution`").Get<size_t>(0);
	size_t maxindex = d_database("SELECT MAX(`index`) FROM `solution`").Get<size_t>(0);

	Get<Gtk::Range>("hscale_iteration")->set_range(0, maxiteration + 1);
	Get<Gtk::Range>("hscale_solution")->set_range(0, maxindex + 1);

	SolutionChanged();
}

void
Window::FillKeyValue(Glib::RefPtr<Gtk::ListStore> store, string const &table, string const &keyname, string const &valuename, string const &condition)
{
	store->clear();

	stringstream q;

	q << "SELECT `" << keyname << "`, `" << valuename << "` FROM `" << table << "`";

	if (condition != "")
	{
		q << " WHERE " << condition;
	}

	q << " ORDER BY `" << keyname << "`";

	sqlite::Row row = d_database(q.str());

	if (row.Done())
	{
		return;
	}

	while (row)
	{
		Gtk::TreeRow iter = *(store->append());

		iter.set_value(0, row.Get<string>(0));

		try
		{
			iter.set_value(1, row.Get<string>(1));
		}
		catch (exception &e)
		{
		}

		row.Next();
	}
}

void
Window::FillKeyValue(string const &storeName, string const &table, string const &keyname, string const &valuename, string const &condition)
{
	FillKeyValue(Get<Gtk::ListStore>(storeName), table, keyname, valuename, condition);
}

void
Window::FillLog()
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_log");

	store->clear();
	sqlite::Row row = d_database("SELECT time, type, message FROM log");

	if (row.Done())
	{
		return;
	}

	while (row)
	{
		Gtk::TreeRow r = *(store->append());

		r->set_value(0, FormatDate(row.Get<size_t>(0)));
		r->set_value(1, row.Get<string>(1));
		r->set_value(2, row.Get<string>(2));

		row.Next();
	}
}

void Window::FillOverrides()
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_override");

	store->clear();
	sqlite::Row row = d_database("SELECT `name`, `value` FROM optiextractor_overrides");

	if (row.Done())
	{
		return;
	}

	while (row)
	{
		Gtk::TreeRow r = *(store->append());

		r->set_value(0, row.Get<string>(0));
		r->set_value(1, row.Get<string>(1));

		row.Next();
	}
}

string
Window::FormatDate(size_t timestamp, std::string const &format) const
{
	char buffer[1024];
	time_t t = static_cast<time_t>(timestamp);

	struct tm *tt = localtime(&t);
	strftime(buffer, 1024, format.c_str(), tt);

	return string(buffer);
}

void Window::HandleRunnerStarted()
{
	// Don't really need to do anything...
}

void
Window::HandleRunnerStopped()
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

	dialog->set_transient_for(*d_window);

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

		for (int i = 0; i < d_lastResponse.fitness_size(); ++i)
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
Window::InitializeUI()
{
	try
	{
		d_builder = Glib::RefPtr<Gtk::Builder>(Gtk::Builder::create_from_file(DATADIR "/optiextractor/window.xml"));
	}
	catch (Gtk::BuilderError &error)
	{
		cerr << "Could not construct interface: " << error.what() << endl;
		exit(1);
	}

	d_builder->get_widget("window", d_window);

	/* Create menu */
	d_uiManager = Gtk::UIManager::create();

	d_actionGroup = Gtk::ActionGroup::create();
	d_actionGroup->add(Gtk::Action::create("FileMenuAction", "_File"));
	d_actionGroup->add(Gtk::Action::create("FileOpenAction", Gtk::Stock::OPEN),
	                   sigc::mem_fun(*this, &Window::OnFileOpen));
	d_actionGroup->add(Gtk::Action::create("FileCloseAction", Gtk::Stock::CLOSE),
	                   sigc::mem_fun(*this, &Window::OnFileClose));
	d_actionGroup->add(Gtk::Action::create("FileQuitAction", Gtk::Stock::QUIT),
	                   sigc::mem_fun(*this, &Window::OnFileQuit));

	d_actionGroup->add(Gtk::Action::create("DatabaseMenuAction", "_Database"));
	d_actionGroup->add(Gtk::Action::create("DatabaseExportAction", "_Export"),
	                   sigc::mem_fun(*this, &Window::OnDatabaseExport));

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
	"    </menu>"
	"  </menubar>"
	"</ui>";

	d_uiManager->add_ui_from_string(ui_info);

	Gtk::Widget *menu = d_uiManager->get_widget("/ui/MenuBar");
	Get<Gtk::VBox>("vbox_main")->pack_start(*menu, Gtk::PACK_SHRINK);

	Get<Gtk::Range>("hscale_solution")->signal_value_changed().connect(sigc::mem_fun(*this, &Window::SolutionChanged));
	Get<Gtk::Range>("hscale_iteration")->signal_value_changed().connect(sigc::mem_fun(*this, &Window::SolutionChanged));

	Get<Gtk::Button>("button_execute")->signal_clicked().connect(sigc::mem_fun(*this, &Window::ExecuteClicked));

	Get<Gtk::Button>("button_add_override")->signal_clicked().connect(sigc::mem_fun(*this, &Window::OverrideAdd));
	Get<Gtk::Button>("button_remove_override")->signal_clicked().connect(sigc::mem_fun(*this, &Window::OverrideRemove));

	Get<Gtk::CellRendererText>("cell_renderer_text_override_name")->signal_edited().connect(sigc::mem_fun(*this, &Window::OverrideNameEdited));

	Get<Gtk::CellRendererText>("cell_renderer_text_override_value")->signal_edited().connect(sigc::mem_fun(*this, &Window::OverrideValueEdited));

	Clear();
}

void
Window::OnDatabaseExport()
{
	Gtk::FileChooserDialog *dialog;

	dialog = new Gtk::FileChooserDialog(*d_window,
	                                   "Export Optimization Database",
	                                    Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog->add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

	dialog->set_local_only(true);

	Gtk::FileFilter *txtfilter = new Gtk::FileFilter();

	txtfilter->set_name("Matlab data text files (*.txt)");
	txtfilter->add_pattern("*.txt");
	dialog->add_filter(*txtfilter);

	Gtk::FileFilter *filter = new Gtk::FileFilter();
	filter->set_name("All files (*)");
	filter->add_pattern("*");
	dialog->add_filter(*filter);
	delete filter;

	if (dialog->run() != Gtk::RESPONSE_OK)
	{
		delete txtfilter;
		delete dialog;

		return;
	}

	string filename = dialog->get_filename();
	if (dialog->get_filter() && dialog->get_filter()->get_name() == txtfilter->get_name())
	{
		if (!String(filename).EndsWith(".txt"))
		{
			filename += ".txt";
		}
	}

	delete txtfilter;
	delete dialog;

	ofstream fstr(filename.c_str(), ios::out);

	if (!fstr)
	{
		Error("Could not create file to export to");
		return;
	}

	size_t iterations = d_database("SELECT COUNT(*) FROM `solution` GROUP BY `index`").Get<size_t>(0);
	size_t solutions = d_database("SELECT COUNT(*) FROM `solution`").Get<size_t>(0);

	/* Collect fitness names */
	sqlite::Row cols = d_database("PRAGMA table_info(`fitness`)");
	vector<string> fitnesses;
	vector<string> colnames;

	fitnesses.push_back("value");
	colnames.push_back("`fitness`.value");

	if (!cols.Done())
	{
		while (cols)
		{
			String name = cols.Get<string>(1);

			if (name.StartsWith("_"))
			{
				fitnesses.push_back(name);
				colnames.push_back("`fitness`." + name);
			}

			cols.Next();
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

	string names = String::Join(colnames, ", ");

	string optimizer = d_database("SELECT `optimizer` FROM `job`").Get<string>(0);
	bool ispso = optimizer == "PSO" || optimizer == "DNPSO";

	sqlite::Row row = d_database(string("SELECT solution.`iteration`, solution.`index`, `values`, `value_names`") + (ispso ? ", `_velocity`" : "") + ", " + names + " FROM `solution` LEFT JOIN `fitness` ON (fitness.iteration = solution.iteration AND fitness.`index` = solution.`index`) ORDER BY solution.`iteration`, solution.`index`");

	bool header = false;

	if (!row.Done())
	{
		while (row)
		{
			if (!header)
			{
				size_t num = String(row.Get<string>(2)).Split(",").size();
				string nm = String::Join(String(row.Get<string>(3)).Split(","), "\t");
				string fitnm = String::Join(fitnesses, "\t");

				fstr << num << "\t" << iterations << "\t" << solutions << "\t" << nm << "\t" << fitnm << endl;
				fstr << String::Join(boundaries, "\t") << endl;
				fstr << String::Join(parameters, "\t") << endl;

				header = true;
			}

			fstr << row.Get<size_t>(0) << "\t" << row.Get<size_t>(1);
			size_t start = ispso ? 5 : 4;

			for (size_t i = start; i < start + fitnesses.size(); ++i)
			{
				fstr << row.Get<string>(i);
			}

			fstr << String::Join(String(row.Get<string>(2)).Split(","), "\t");

			if (ispso)
			{
				fstr << String::Join(String(row.Get<string>(4)).Split(","), "\t");
			}

			fstr << endl;

			row.Next();
		}
	}

	fstr.close();
}

void
Window::OnFileClose()
{
	Clear();
}

void
Window::OnFileOpen()
{
	/* Open dialog */
	if (d_openDialog)
	{
		delete d_openDialog;
	}

	d_openDialog = new Gtk::FileChooserDialog(*d_window,
	                                          "Open Optimization Database",
	                                          Gtk::FILE_CHOOSER_ACTION_OPEN);

	d_openDialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	d_openDialog->add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	d_openDialog->set_local_only(true);

	d_openDialog->signal_response().connect(sigc::mem_fun(*this, &Window::OpenResponse));
	d_openDialog->show();
}

void
Window::OnFileQuit()
{
	Gtk::Main::quit();
}

void
Window::Open(sqlite::SQLite database)
{
	Clear();

	d_database = database;

	if (!d_database)
	{
		Error("<b>Database could not be opened</b>", "Please make sure the file is a valid optimization results database.");
		return;
	}

	// Do a quick test query
	sqlite::Row row = d_database("PRAGMA table_info(settings)");

	if (!d_database("PRAGMA quick_check") || (!row || row.Done()))
	{
		Error("<b>Database could not be opened</b>", ". Please make sure the file is a valid optimization results database.");
		return;
	}

	// Make sure to create the optiextractor_override table to store overrides
	row = d_database("PRAGMA table_info(optiextractor_overrides)");

	if (!row || row.Done())
	{
		d_database("CREATE TABLE `optiextractor_overrides` (`name` TEXT, `value` TEXT)");
	}

	Fill();
}

void
Window::Open(string const &filename)
{
	Clear();

	if (!FileSystem::FileExists(filename))
	{
		Error("<b>Database file does not exist</b>", "The file '<i>" + filename + "</i>' does not exist. Please make sure you open a valid optimization results database.");
		return;
	}

	Open(sqlite::SQLite(filename));
}

void
Window::OpenResponse(int response)
{
	if (response == Gtk::RESPONSE_OK)
	{
		Open(d_openDialog->get_filename());
	}

	delete d_openDialog;
	d_openDialog = 0;
}

void
Window::OverrideAdd()
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

	d_database("DELETE FROM `optiextractor_overrides` WHERE `name` = 'key'");
	d_database("INSERT INTO `optiextractor_overrides` (`name`, `value`) VALUES ('key', 'value')");
}

void
Window::OverrideNameEdited(Glib::ustring const &path, Glib::ustring const &newtext)
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_override");

	Gtk::TreeModel::iterator iter = store->get_iter(path);
	Gtk::TreeRow row = *iter;

	string current;

	row->get_value(0, current);
	row->set_value(0, newtext);

	d_database() << "UPDATE `optiextractor_overrides` SET `name` = '"
	             << sqlite::SQLite::Serialize(newtext)
	             << "' WHERE `name` = '"
	             << sqlite::SQLite::Serialize(current)
	             << "'"
	             << sqlite::SQLite::Query::End();
}

void
Window::OverrideRemove()
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

	d_database() << "DELETE FROM optiextractor_overrides WHERE `name` = '"
	             << sqlite::SQLite::Serialize(name) << "'"
	             << sqlite::SQLite::Query::End();

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
Window::OverrideValueEdited(Glib::ustring const &path, Glib::ustring const &newtext)
{
	Glib::RefPtr<Gtk::ListStore> store = Get<Gtk::ListStore>("list_store_override");

	Gtk::TreeModel::iterator iter = store->get_iter(path);
	Gtk::TreeRow row = *iter;

	string current;

	row->get_value(0, current);
	row->set_value(1, newtext);

	d_database() << "UPDATE `optiextractor_overrides` SET `value` = '"
	             << sqlite::SQLite::Serialize(newtext)
	             << "' WHERE `name` = '"
	             << sqlite::SQLite::Serialize(current) << "'"
	             << sqlite::SQLite::Query::End();
}

void
Window::RunnerResponse(Response const &response)
{
	d_lastResponse = response;
}

void
Window::RunnerState(bool running)
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
Window::RunSolution()
{
	d_lastResponse = Response();
	d_lastResponse.set_status(Response::Failed);
	d_lastResponse.mutable_failure()->set_type(Response::Failure::NoResponse);

	if (!d_runner.Run(d_database, d_iterationId, d_solutionId))
	{
		Error("Could not run solution: " + d_runner.Error());
	}
}

void
Window::SolutionChanged()
{
	Get<Gtk::Label>("label_solution_iteration")->set_text("");
	Get<Gtk::Label>("label_solution_solution")->set_text("");

	Glib::RefPtr<Gtk::ListStore> store_solutions = Get<Gtk::ListStore>("list_store_solutions");
	store_solutions->clear();

	Glib::RefPtr<Gtk::ListStore> store_fitness = Get<Gtk::ListStore>("list_store_solution_fitness");
	store_fitness->clear();

	d_solutionId = 0;
	d_iterationId = 0;

	if (!Get<Gtk::Notebook>("notebook")->sensitive())
	{
		return;
	}

	sqlite::Row names = d_database("PRAGMA table_info(`parameter_values`)");
	vector<string> cols;
	string colnames;

	while (names && !names.Done())
	{
		string name = names.Get<string>(1);
		names.Next();

		if (!String(name).StartsWith("_p_"))
		{
			continue;
		}

		cols.push_back(name.substr(3));

		if (colnames != "")
		{
			colnames += ", ";
		}

		colnames += "parameter_values.`" + name + "`";
	}

	size_t iteration = Get<Gtk::Range>("hscale_iteration")->get_value();
	size_t solution = Get<Gtk::Range>("hscale_solution")->get_value();

	stringstream q;

	if (colnames != "")
	{
		colnames = ", " + colnames;
	}

	sqlite::Row paramExists = d_database("PRAGMA table_info(parameter_values)");
	string join;

	if (paramExists && !paramExists.Done())
	{
		join = "LEFT JOIN parameter_values ON (parameter_values.`iteration` = solution.`iteration` AND parameter_values.`index` = solution.`index`)";
	}

	q << "SELECT solution.`iteration`, solution.`index`, solution.`fitness` " << colnames << " FROM solution " << join;

	if (solution == 0 && iteration != 0)
	{
		q << " WHERE solution.`iteration` = " << (iteration - 1);
	}
	else if (iteration == 0 && solution != 0)
	{
		q << " WHERE solution.`index` = " << (solution - 1);
	}
	else if (iteration != 0 && solution != 0)
	{
		q << " WHERE solution.`iteration` = " << (iteration - 1) << " AND solution.`index` = " << (solution - 1);
	}

	q << " ORDER BY `fitness` DESC LIMIT 1";

	sqlite::Row row = d_database(q.str());

	if (row.Done())
	{
		return;
	}

	{
		d_iterationId = row.Get<size_t>(0);

		stringstream s;
		s << (d_iterationId + 1);

		Get<Gtk::Label>("label_solution_iteration")->set_text(s.str());
	}

	{
		d_solutionId = row.Get<size_t>(1);
		stringstream s;

		s << (d_solutionId + 1);

		Get<Gtk::Label>("label_solution_solution")->set_text(s.str());
	}

	for (size_t i = 0; i < cols.size(); ++i)
	{
		Gtk::TreeRow r = *(store_solutions->append());

		string name = cols[i];
		string value = row.Get<string>(i + 3);

		r.set_value(0, name);
		r.set_value(1, value);
	}

	row = d_database() << "SELECT * FROM fitness"
	                   << " WHERE `iteration` = " << d_iterationId
	                   << " AND `index` = " << d_solutionId
	                   << sqlite::SQLite::Query::End();

	if (row && !row.Done())
	{
		names = d_database("PRAGMA table_info(`fitness`)");

		while (names && !names.Done())
		{
			string name = names.Get<string>(1);

			if (String(name).StartsWith("_f_") || name == "value")
			{
				Gtk::TreeRow r = *(store_fitness->append());

				if (name != "value")
				{
					r.set_value(0, name.substr(3));
				}
				else
				{
					r.set_value(0, string("Fitness"));
				}

				r.set_value(1, row.Get<string>(name));
			}

			names.Next();
		}
	}
}

Gtk::Window &
Window::GtkWindow()
{
	return *d_window;
}

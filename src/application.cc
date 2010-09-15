#include "application.hh"
#include "window.hh"
#include "exporter.hh"

#include <jessevdk/os/os.hh>
#include <jessevdk/db/db.hh>
#include <iomanip>

using namespace optiextractor;
using namespace std;
using namespace jessevdk::os;
using namespace jessevdk::db;
using namespace optimization::messages::task;

Application::Application(int &argc, char **&argv)
:
	d_run(false),
	d_iteration(-1),
	d_solution(-1),
	d_responded(false),
	d_export(false)
{
	ParseArguments(argc, argv);
}

void
Application::Run(int &argc, char **&argv)
{
	if (d_overrideSetting != "")
	{
		RunOverride(argc, argv);
	}
	else if (d_run)
	{
		RunDispatcher(argc, argv);
	}
	else if (d_export)
	{
		RunExporter(argc, argv);
	}
	else
	{
		RunWindow(argc, argv);
	}
}

void
Application::RunExporter(int &argc, char **&argv)
{
	if (argc <= 1)
	{
		cerr << "Please provide a database" << endl;
		return;
	}

	for (int i = 1; i < argc; ++i)
	{
		string filename = argv[i];
		string outfile;

		if (d_exportout != "")
		{
			if (argc > 2)
			{
				stringstream s;
				s << d_exportout << "." << i;

				outfile = s.str();
			}
			else
			{
				outfile = d_exportout;
			}
		}
		else
		{
			outfile = filename + ".mat";
		}

		RunExporter(argv[i], outfile);
	}
}

void
Application::ExporterProgress(double progress)
{
	cout << setw(4) << static_cast<size_t>(progress * 100) << " %\r";
}

void
Application::RunExporter(string const &filename, string const &outfile)
{
	if (!FileSystem::FileExists(filename))
	{
		cerr << "The file '" + filename + "' does not exist. Please make sure you open a valid optimization results database." << endl;
		return;
	}

	sqlite::SQLite database(filename);

	if (!database)
	{
		cerr << "Database '" << filename << "'could not be opened. Please make sure the file is a valid optimization results database." << endl;
		return;
	}

	// Do a quick test query
	sqlite::Row row = database("PRAGMA table_info(settings)");

	if (!row || row.Done())
	{
		cerr << "Database could not be opened. The file '" << filename << "' could not be opened. Please make sure the file is a valid optimization results database." << endl;
		return;
	}

	Exporter exporter(outfile, database);

	exporter.OnProgress.Add(*this, &Application::ExporterProgress);
	exporter.Export();

	cout << endl;
}

void
Application::RunDispatcher(int &argc, char **&argv)
{
	if (d_iteration < 0)
	{
		cerr << "Please specify an iteration" << endl;
		return;
	}

	if (d_solution < 0)
	{
		cerr << "Please specify a solution" << endl;
		return;
	}

	if (argc <= 1)
	{
		cerr << "Please provide a database" << endl;
		return;
	}

	string filename = argv[1];

	sqlite::SQLite database(filename);

	if (!database)
	{
		cerr << "Could not open database: " << filename << endl;
		return;
	}

	d_runner.OnState.Add(*this, &Application::RunnerState);
	d_runner.OnResponse.Add(*this, &Application::RunnerResponse);

	d_runner.Run(database, d_iteration, d_solution);

	d_main = Glib::MainLoop::create(false);
	d_main->run();
}

void
Application::RunnerState(bool running)
{
	if (!running)
	{
		if (!d_responded)
		{
			cerr << "No response";

			if (d_runner.Error() != "")
			{
				cerr << ": " << d_runner.Error();
			}

			cerr << endl;
		}

		d_main->quit();
	}
}

void
Application::RunnerResponse(Response response)
{
	if (response.status() == Response::Failed)
	{
		string message;

		switch (response.failure().type())
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

		string error = d_runner.Error();

		if (response.failure().message() != "")
		{
			if (error != "")
			{
				error += "\n\n";
			}

			error += response.failure().message();
		}

		cerr << "Solution failed: " << message << endl;

		if (error != "")
		{
			cerr << "Error: " << error << endl;
		}
	}
	else
	{
		stringstream fitness;

		for (int i = 0; i < response.fitness_size(); ++i)
		{
			if (i != 0)
			{
				fitness << ", ";
			}

			fitness << response.fitness(i).name() << " = " << response.fitness(i).value();
		}

		cout << "Solution success: " << fitness.str() << endl;
	}

	d_responded = true;
}

void
Application::RunWindow(int &argc, char **&argv)
{
	Gtk::Main loop(argc, argv);

	Window window;

	if (argc > 1)
	{
		window.Open(argv[1]);
	}

	loop.run(window.GtkWindow());
}

void
Application::RunOverride(int &argc, char **&argv)
{
	if (argc <= 1)
	{
		cerr << "Please provide a database" << endl;
	}

	for (int i = 1; i < argc; ++i)
	{
		RunOverride(argv[i]);
	}
}

void
Application::RunOverride(string const &filename)
{
	if (!FileSystem::FileExists(filename))
	{
		cerr << "The file '" + filename + "' does not exist. Please make sure you open a valid optimization results database." << endl;
		return;
	}

	sqlite::SQLite database(filename);

	if (!database)
	{
		cerr << "Database '" << filename << "'could not be opened. Please make sure the file is a valid optimization results database." << endl;
		return;
	}

	// Do a quick test query
	sqlite::Row row = database("PRAGMA table_info(settings)");

	if (!row || row.Done())
	{
		cerr << "Database could not be opened. The file '" << filename << "' could not be opened. Please make sure the file is a valid optimization results database." << endl;
		return;
	}

	// Make sure to create the optiextractor_override table to store overrides
	row = database("PRAGMA table_info(optiextractor_overrides)");

	if (!row || row.Done())
	{
		database("CREATE TABLE `optiextractor_overrides` (`name` TEXT, `value` TEXT)");
	}

	cout << "Setting '" << d_overrideSetting << "' in '" << filename << "'..." << endl;

	// Delete the possible previous setting
	database() << "DELETE FROM `optiextractor_overrides` WHERE "
	           << "`name` = '" << sqlite::SQLite::Serialize(d_overrideSetting) << "'"
	           << sqlite::SQLite::Query::End();

	if (d_overrideValue != "")
	{
		database() << "INSERT INTO `optiextractor_overrides` (`name`, `value`) VALUES ("
		           << "'" << sqlite::SQLite::Serialize(d_overrideSetting) << "', "
		           << "'" << sqlite::SQLite::Serialize(d_overrideValue) << "')"
		           << sqlite::SQLite::Query::End();
	}
}

void
Application::ParseArguments(int &argc, char **&argv)
{
	Glib::OptionGroup group("optiextractor", "Optimization Results Extractor");

	// Override setting
	Glib::OptionEntry overrideSetting;

	overrideSetting.set_long_name("override-setting");
	overrideSetting.set_description("Override setting in database");

	group.add_entry(overrideSetting, d_overrideSetting);

	// Override value
	Glib::OptionEntry overrideValue;
	overrideValue.set_long_name("override-value");
	overrideValue.set_description("New value of the override setting (see --override-setting)");

	group.add_entry(overrideValue, d_overrideValue);

	// Run solution
	Glib::OptionEntry run;
	run.set_long_name("run");
	run.set_short_name('r');
	run.set_description("Run a solution from the database");

	group.add_entry(run, d_run);

	// Iteration number
	Glib::OptionEntry iteration;
	iteration.set_long_name("iteration");
	iteration.set_short_name('i');
	iteration.set_description("The iteration to run");

	group.add_entry(iteration, d_iteration);

	// Solution number
	Glib::OptionEntry solution;
	solution.set_long_name("solution");
	solution.set_short_name('s');
	solution.set_description("The solution to run");

	group.add_entry(solution, d_solution);

	// Export database
	Glib::OptionEntry exprt;
	exprt.set_long_name("export");
	exprt.set_short_name('e');
	exprt.set_description("Export the database to a text file");

	group.add_entry(exprt, d_export);

	// Export database output
	Glib::OptionEntry exprtout;
	exprtout.set_long_name("output");
	exprtout.set_short_name('o');
	exprtout.set_description("Export output file");

	group.add_entry(exprtout, d_exportout);

	Glib::OptionContext context;

	context.set_main_group(group);
	context.parse(argc, argv);
}

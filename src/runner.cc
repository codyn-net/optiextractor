#include "runner.hh"
#include "dispatcher.hh"
#include "utils.hh"

#include <optimization/messages.hh>

using namespace std;
using namespace optimization::messages::task;
using namespace jessevdk::base;
using namespace jessevdk::os;
using namespace jessevdk::db;
using namespace optimization;
using namespace optiextractor;

void
Runner::Cancel()
{
	if (d_running)
	{
		DispatchClose();
	}
}

void
Runner::DispatchClose()
{
	d_pipe.ReadEnd().OnData().Remove(*this, &Runner::OnDispatchData);
	d_pipe.ReadEnd().Close();
	d_pipe.WriteEnd().Close();

	d_errorChannel.OnData().Remove(*this, &Runner::OnErrorData);
	d_errorChannel.Close();

	d_connection.disconnect();

	d_running = false;

	OnState(d_running);
	d_errorMessage = "";
}

bool
Runner::OnDispatchData(FileDescriptor::DataArgs &args)
{
	vector<Communication> messages;
	vector<Communication>::iterator iter;

	Messages::Extract(args, messages);

	for (iter = messages.begin(); iter != messages.end(); ++iter)
	{
		if (iter->type() == Communication::CommunicationResponse)
		{
			Response resp = iter->response();

			OnResponse(resp);
		}

		break;
	}

	return false;
}

void
Runner::OnDispatchedKilled(GPid pid, int ret)
{
	Glib::spawn_close_pid(pid);

	DispatchClose();
}

bool
Runner::OnErrorData(FileDescriptor::DataArgs &args)
{
	d_errorMessage += args.data;
	return false;
}

void
Runner::FillData(sqlite::SQLite database, size_t iteration, size_t solution, Task &task)
{
	// Set data
	sqlite::Row data = database() << "SELECT * FROM `data` WHERE iteration = "
	                              << iteration << " AND `index` = "
	                              << solution << sqlite::SQLite::Query::End();

	if (data && !data.Done())
	{
		sqlite::Row header = database("PRAGMA table_info(`data`)");
		int i = 0;

		while (header && !header.Done())
		{
			string name = header.Get<string>(1);

			if (String(name).StartsWith("_d_"))
			{
				Task::KeyValue *kv = task.add_data();

				kv->set_key(name.substr(3));
				kv->set_value(data.Get<string>(i));
			}

			header.Next();
			++i;
		}
	}
}

void
Runner::FillParameters(sqlite::SQLite database, size_t iteration, size_t solution, Task &task)
{
	vector<string> cols = Utils::ActiveParameters(database, iteration, solution);
	string colnames;

	for (size_t i = 0; i < cols.size(); ++i)
	{
		if (i != 0)
		{
			colnames += ", ";
		}

		colnames += "parameter_values.`" + cols[i] + "`";
	}

	if (colnames == "")
	{
		return;
	}

	sqlite::Row row(0, 0);

	row = database() << "SELECT " << colnames << " FROM `parameter_values` WHERE "
	                 << "parameter_values.`iteration` = " << iteration << " AND "
	                 << "parameter_values.`index` = " << solution
	                 << sqlite::SQLite::Query::End();

	if (!row || row.Done())
	{
		return;
	}

	for (size_t i = 0; i < row.Length(); ++i)
	{
		Task::Parameter *parameter = task.add_parameters();

		string name = cols[i].substr(3);

		parameter->set_name(name);
		parameter->set_value(row.Get<double>(i));

		sqlite::Row row = database() << "SELECT `min`, `max` FROM boundaries WHERE "
		                             << "boundaries.name = (SELECT parameters.boundary FROM parameters "
		                             << " WHERE name = '" << name << "')"
		                             << sqlite::SQLite::Query::End();

		if (row && !row.Done())
		{
			parameter->set_min(row.Get<double>(0));
			parameter->set_max(row.Get<double>(1));
		}
		else
		{
			cerr << "Could not read parameter boundary: '" << name << "'" << endl;
			
			parameter->set_min(0);
			parameter->set_max(0);
		}
	}
}

bool
Runner::Run(sqlite::SQLite database, size_t iteration, size_t solution)
{
	Cancel();

	Task task;
	string dispatcher;

	sqlite::Row job = database("SELECT name, optimizer, dispatcher FROM job");

	task.set_job(job.Get<string>(0));
	task.set_optimizer(job.Get<string>(1));
	task.set_id(solution);

	dispatcher = job.Get<string>(2);

	FillParameters(database, iteration, solution, task);
	FillData(database, iteration, solution, task);

	// Dispatcher settings
	std::map<std::string, std::string> settings;

	settings["optiextractor"] = "yes";

	sqlite::Row row = database("SELECT `name`, `value` FROM dispatcher");

	while (row && !row.Done())
	{
		string name = row.Get<string>(0);
		string value = row.Get<string>(1);

		settings[name] = value;
		row.Next();
	}

	// Override settings
	row = database("SELECT `name`, `value` FROM optiextractor_overrides");

	while (row && !row.Done())
	{
		string name = row.Get<string>(0);
		string value = row.Get<string>(1);

		if (name == "dispatcher")
		{
			dispatcher = value;
		}
		else
		{
			settings[name] = value;
		}

		row.Next();
	}

	for (std::map<string, string>::iterator it = settings.begin(); it != settings.end(); ++it)
	{
		Task::KeyValue *setting;
		setting = task.add_settings();

		setting->set_key(it->first);
		setting->set_value(it->second);
	}

	task.set_dispatcher(dispatcher);

	return Run(task, Dispatcher::Resolve(dispatcher));
}

string
Runner::ExpandVariables(string const &s)
{
	string ret;
	size_t pos = 0;

	while (pos < s.size())
	{
		size_t epos = s.find_first_of('$', pos);

		if (epos == string::npos)
		{
			break;
		}

		size_t end = epos + 1;

		while (end < s.size() && (isalnum(s[end]) || s[end] == '_'))
		{
			++end;
		}

		string val = Glib::getenv(s.substr(epos + 1, end - epos - 2));

		ret += s.substr(pos, epos - pos) + val;
		pos = end;
	}

	if (pos < s.size())
	{
		ret += s.substr(pos);
	}

	return ret;
}

bool
Runner::Run(Task const &task, string const &executable)
{
	Cancel();

	if (executable == "")
	{
		d_errorMessage = "Could not find dispatcher";
		return false;
	}

	vector<string> argv;
	argv.push_back(executable);

	Glib::Pid pid;
	int stdin;
	int stdout;
	int stderr;

	/* Check for environment variable setting */
	size_t num = task.settings_size();
	map<string, string> env = Environment::All();

	for (size_t i = 0; i < num; ++i)
	{
		Task::KeyValue const &kv = task.settings(i);

		if (kv.key() != "environment")
		{
			continue;
		}

		vector<string> vars = String(kv.value()).Split(",");

		for (vector<string>::iterator iter = vars.begin(); iter != vars.end(); ++iter)
		{
			vector<string> parts = String(*iter).Strip().Split("=", 2);
			string key = String(parts[0]).Strip();

			if (parts.size() == 2)
			{
				env[key] = ExpandVariables(String(parts[1]).Strip());
			}
			else if (parts.size() == 1)
			{
				env[key] = "";
			}
		}
	}

	try
	{
		Glib::spawn_async_with_pipes(FileSystem::Dirname(executable),
		                             argv,
		                             Environment::Convert(env),
		                             Glib::SPAWN_DO_NOT_REAP_CHILD,
		                             sigc::slot<void>(),
		                             &pid,
		                             &stdin,
		                             &stdout,
		                             &stderr);
	}
	catch (Glib::SpawnError &e)
	{
		d_errorMessage = "Could not spawn dispatcher: " + e.what();
		return false;
	}

	/* Create pipe */
	d_pipe = Pipe(stdout, stdin);
	d_errorChannel = FileDescriptor(stderr);
	d_errorChannel.Attach();

	d_errorMessage = "";

	d_running = true;
	OnState(d_running);

	d_connection = Glib::signal_child_watch().connect(sigc::mem_fun(*this, &Runner::OnDispatchedKilled), pid);

	d_pipe.ReadEnd().OnData().Add(*this, &Runner::OnDispatchData);
	d_errorChannel.OnData().Add(*this, &Runner::OnErrorData);

	WriteTask(task);
	return true;
}

Runner::Runner()
:
	d_running(false)
{
}

void
Runner::WriteTask(Task const &task)
{
	string serialized;

	Communication comm;

	comm.set_type(Communication::CommunicationTask);
	*comm.mutable_task() = task;

	Messages::Create(comm, serialized);
	d_pipe.WriteEnd().Write(serialized);
}

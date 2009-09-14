#include "runner.ih"

bool Runner::run(Request::Dispatch const &dispatch, string const &executable)
{
	if (d_running)
	{
		dispatchClose();
	}
	
	vector<string> argv;
	argv.push_back(executable);

	Glib::Pid pid;
	int stdin;
	int stdout;
	int stderr;
	
	/* Check for environment variable setting */
	size_t num = dispatch.settings_size();
	map<string, string> env = Environment::all();
	
	for (size_t i = 0; i < num; ++i)
	{
		Request::Dispatch::KeyValue const &kv = dispatch.settings(i);

		if (kv.key() != "environment")
		{
			continue;
		}

		vector<string> vars = String(kv.value()).split(",");
		
		for (vector<string>::iterator iter = vars.begin(); iter != vars.end(); ++iter)
		{
			vector<string> parts = String(*iter).strip().split("=", 2);
			string key = String(parts[0]).strip();
			
			if (parts.size() == 2)
			{
				env[key] = String(parts[1]).strip();
			}
			else if (parts.size() == 1)
			{
				env[key] = "";
			}
		}
	}

	try
	{
		Glib::spawn_async_with_pipes(FileSystem::dirname(executable),
				                     argv,
				                     Environment::convert(env),
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
	d_errorChannel.attach();

	d_errorMessage = "";
	
	d_running = true;
	onState(d_running);

	d_connection = Glib::signal_child_watch().connect(sigc::mem_fun(*this, &Runner::onDispatchedKilled), pid);

	d_pipe.readEnd().onData().add(*this, &Runner::onDispatchData);
	d_errorChannel.onData().add(*this, &Runner::onErrorData);

	writeTask(dispatch);
	return true;
}

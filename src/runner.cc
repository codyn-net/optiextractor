#include "runner.hh"

#include <optimization/messages.hh>

using namespace std;
using namespace optimization::messages::task;
using namespace base;
using namespace os;
using namespace optimization;

void
Runner::Cancel()
{
	if (d_running)
	{
		Response response;
		
		response.mutable_failure()->set_type(Response::Failure::Disconnected);
		DispatchClose();
	}
}

void
Runner::DispatchClose()
{
	d_pipe.readEnd().onData().remove(*this, &Runner::OnDispatchData);
	d_pipe.readEnd().close();
	d_pipe.writeEnd().close();

	d_errorChannel.onData().remove(*this, &Runner::OnErrorData);
	d_errorChannel.close();

	d_connection.disconnect();
	
	d_running = false;
	
	OnState(d_running);
	d_errorMessage = "";
}

bool
Runner::OnDispatchData(os::FileDescriptor::DataArgs &args)
{
	vector<Response> messages;
	vector<Response>::iterator iter;
	
	Messages::Extract(args, messages);
	
	for (iter = messages.begin(); iter != messages.end(); ++iter)
	{
		OnResponse(*iter);
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
Runner::OnErrorData(os::FileDescriptor::DataArgs &args)
{
	d_errorMessage += args.data;
	return false;
}

bool
Runner::Run(Task::Description const &description, string const &executable)
{
	if (d_running)
	{
		DispatchClose();
	}
	
	vector<string> argv;
	argv.push_back(executable);

	Glib::Pid pid;
	int stdin;
	int stdout;
	int stderr;
	
	/* Check for environment variable setting */
	size_t num = description.settings_size();
	map<string, string> env = Environment::all();
	
	for (size_t i = 0; i < num; ++i)
	{
		Task::Description::KeyValue const &kv = description.settings(i);

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
	OnState(d_running);

	d_connection = Glib::signal_child_watch().connect(sigc::mem_fun(*this, &Runner::OnDispatchedKilled), pid);

	d_pipe.readEnd().onData().add(*this, &Runner::OnDispatchData);
	d_errorChannel.onData().add(*this, &Runner::OnErrorData);

	WriteTask(description);
	return true;
}

Runner::Runner()
:
	d_running(false)
{
}

void
Runner::WriteTask(Task::Description const &description)
{
	string serialized;

	Messages::Create(description, serialized);
	d_pipe.writeEnd().write(serialized);
}

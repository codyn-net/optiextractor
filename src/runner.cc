#include "runner.hh"

#include <optimization/messages.hh>

using namespace std;
using namespace optimization::messages::task;
using namespace jessevdk::base;
using namespace jessevdk::os;
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
			OnResponse(iter->response());
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

bool
Runner::Run(Task const &task, string const &executable)
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
				env[key] = String(parts[1]).Strip();
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

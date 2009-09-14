#ifndef __RUNNER_H__
#define __RUNNER_H__

#include <base/base.hh>
#include <optimization/Messages/messages.hh>
#include <os/os.hh>

class Runner
{
	std::string d_dispatcher;
	std::string d_errorMessage;
	os::Pipe d_pipe;
	sigc::connection d_connection;
	os::FileDescriptor d_errorChannel;
	bool d_running;

	public:
		/* Constructor/destructor */
		Runner();
		
		/* Public functions */
		bool run(optimization::messages::worker::Request::Dispatch const &dispatch, std::string const &executable);
		
		std::string const &error() const;
		operator bool() const;
		
		void cancel();
		
		base::signals::Signal<bool> onState;
		base::signals::Signal<optimization::messages::worker::Response> onResponse;
	private:
		/* Private functions */
		void writeTask(optimization::messages::worker::Request::Dispatch const &dispatch);

		void onDispatchedKilled(GPid pid, int ret);
		bool onDispatchData(os::FileDescriptor::DataArgs &args);
		bool onErrorData(os::FileDescriptor::DataArgs &args);
			
		void dispatchClose();		
};

inline std::string const &Runner::error() const
{
	return d_errorMessage;
}

inline Runner::operator bool() const
{
	return d_running;
}

#endif /* __RUNNER_H__ */

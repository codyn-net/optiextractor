#ifndef __RUNNER_H__
#define __RUNNER_H__

#include <base/base.hh>
#include <optimization/messages.hh>
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
		bool Run(optimization::messages::task::Task::Description const &description, std::string const &executable);
		
		std::string const &Error() const;
		operator bool() const;
		
		void Cancel();
		
		base::signals::Signal<bool> OnState;
		base::signals::Signal<optimization::messages::task::Response> OnResponse;
	private:
		/* Private functions */
		void WriteTask(optimization::messages::task::Task::Description const &description);

		void OnDispatchedKilled(GPid pid, int ret);
		bool OnDispatchData(os::FileDescriptor::DataArgs &args);
		bool OnErrorData(os::FileDescriptor::DataArgs &args);
			
		void DispatchClose();
};

inline std::string const &
Runner::Error() const
{
	return d_errorMessage;
}

inline Runner::operator bool() const
{
	return d_running;
}

#endif /* __RUNNER_H__ */

#ifndef __RUNNER_H__
#define __RUNNER_H__

#include <jessevdk/base/base.hh>
#include <optimization/messages.hh>
#include <jessevdk/os/os.hh>

class Runner
{
	std::string d_dispatcher;
	std::string d_errorMessage;
	jessevdk::os::Pipe d_pipe;
	sigc::connection d_connection;
	jessevdk::os::FileDescriptor d_errorChannel;
	bool d_running;

	public:
		/* Constructor/destructor */
		Runner();

		/* Public functions */
		bool Run(optimization::messages::task::Task const &task, std::string const &executable);

		std::string const &Error() const;
		operator bool() const;

		void Cancel();

		jessevdk::base::signals::Signal<bool> OnState;
		jessevdk::base::signals::Signal<optimization::messages::task::Response const &> OnResponse;
	private:
		/* Private functions */
		void WriteTask(optimization::messages::task::Task const &task);

		void OnDispatchedKilled(GPid pid, int ret);
		bool OnDispatchData(jessevdk::os::FileDescriptor::DataArgs &args);
		bool OnErrorData(jessevdk::os::FileDescriptor::DataArgs &args);

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

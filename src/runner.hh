#ifndef __OPTIEXTRACTOR_RUNNER_H__
#define __OPTIEXTRACTOR_RUNNER_H__

#include <jessevdk/base/base.hh>
#include <optimization/messages.hh>
#include <jessevdk/os/os.hh>
#include <jessevdk/db/db.hh>

namespace optiextractor
{
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
			bool Run(optimization::messages::task::Task const &task, std::string const &dispatcher);
			bool Run(jessevdk::db::sqlite::SQLite database, size_t iteration, size_t solution);

			std::string const &Error() const;
			operator bool() const;

			void Cancel();

			jessevdk::base::signals::Signal<bool> OnState;
			jessevdk::base::signals::Signal<optimization::messages::task::Response> OnResponse;
		private:
			/* Private functions */
			void WriteTask(optimization::messages::task::Task const &task);

			void OnDispatchedKilled(GPid pid, int ret);
			bool OnDispatchData(jessevdk::os::FileDescriptor::DataArgs &args);
			bool OnErrorData(jessevdk::os::FileDescriptor::DataArgs &args);

			void DispatchClose();

			void FillData(jessevdk::db::sqlite::SQLite database, size_t iteration, size_t solution, optimization::messages::task::Task &task);
			void FillParameters(jessevdk::db::sqlite::SQLite database, size_t iteration, size_t solution, optimization::messages::task::Task &task);

			std::string ExpandVariables(std::string const &s);
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

}

#endif /* __OPTIEXTRACTOR_RUNNER_H__ */

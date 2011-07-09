#ifndef __OPTIEXTRACTOR_APPLICATION_H__
#define __OPTIEXTRACTOR_APPLICATION_H__

#include <gtkmm.h>
#include <jessevdk/db/db.hh>
#include <optimization/optimization.hh>
#include <map>
#include <string>

#include "runner.hh"

namespace optiextractor
{
	class Application
	{
		Glib::ustring d_overrideSetting;
		Glib::ustring d_overrideValue;
		bool d_run;
		int d_iteration;
		int d_solution;
		Runner d_runner;
		Glib::RefPtr<Glib::MainLoop> d_main;
		bool d_responded;
		bool d_export;
		Glib::ustring d_exportout;
		size_t d_progresssize;
		size_t d_previousProgress;

		public:
			/* Constructor/destructor */
			Application(int &argc, char **&argv);

			void Run(int &argc, char **&argv);
		private:
			void ParseArguments(int &argc, char **&argv);

			void RunWindow(int &argc, char **&argv);
			void RunOverride(int &argc, char **&argv);
			void RunOverride(std::string const &filename);
			void RunDispatcher(int &argc, char **&argv);
			void RunExporter(int &argc, char **&argv);
			void RunExporter(std::string const &filename, std::string const &outfile);

			void RunnerState(bool running);
			void RunnerResponse(optimization::messages::task::Response response);

			void ExporterProgress(double progress);
	};
}

#endif /* __OPTIEXTRACTOR_APPLICATION_H__ */

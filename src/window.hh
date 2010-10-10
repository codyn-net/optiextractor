#ifndef __OPTIEXTRACTOR_WINDOW_H__
#define __OPTIEXTRACTOR_WINDOW_H__

#include <gtkmm.h>
#include <jessevdk/db/db.hh>
#include <map>
#include <string>

#include "runner.hh"

namespace optiextractor
{
	class Window
	{
		jessevdk::db::sqlite::SQLite d_database;
		Glib::RefPtr<Gtk::Builder> d_builder;
		Gtk::Window *d_window;
		Gtk::MessageDialog *d_dialog;
		std::map<std::string, std::string> d_parameterMap;
	
		Glib::RefPtr<Gtk::UIManager> d_uiManager;
		Glib::RefPtr<Gtk::ActionGroup> d_actionGroup;
	
		Gtk::FileChooserDialog *d_openDialog;
	
		Runner d_runner;
		optimization::messages::task::Response d_lastResponse;
		std::string d_overrideDispatcher;

		bool d_scanned;
		bool d_logFilled;
		bool d_solutionFilled;

		size_t d_solutionId;
		size_t d_iterationId;

		public:
			/* Constructor/destructor */
			Window();

			template <typename T>
			Glib::RefPtr<T> Get(std::string const &name);

			void Open(std::string const &string);

			Gtk::Window &GtkWindow();
		private:
			/* Private functions */
			void Open(jessevdk::db::sqlite::SQLite database, Glib::RefPtr<Gio::File> file);

			void InitializeUI();
			void Error(std::string const &error, std::string const &secondary = "");
	
			void Fill();
		
			void FillKeyValue(std::string const &storeName, std::string const &table, std::string const &keyname, std::string const &valuename, std::string const &condition = "");

			void FillKeyValue(Glib::RefPtr<Gtk::ListStore> store, std::string const &table, std::string const &keyname, std::string const &valuename, std::string const &condition = "");
		
			void FillBoundaries();
			void FillLog();
			void FillOverrides();

			void Clear();
		
			void DestroyDialog(int response);
			void SolutionChanged();
			void ExecuteClicked();
		
			void RunnerState(bool running);
			void RunnerResponse(optimization::messages::task::Response const &response);
		
			void RunSolution();
			void HandleRunnerStarted();
			void HandleRunnerStopped();
		
			std::string ResolveDispatcher(std::string const &dispatcher);
		
			void Scan();
			void Scan(std::string const &directory);
		
			void OverrideAdd();
			void OverrideRemove();
		
			void OverrideNameEdited(Glib::ustring const &path, Glib::ustring const &newtext);
			void OverrideValueEdited(Glib::ustring const &path, Glib::ustring const &newtext);
		
			std::string FormatDate(size_t timestamp, std::string const &format = "%d-%m, %R") const;
		
			void OnFileOpen();
			void OnFileClose();
			void OnFileQuit();

			void OnDatabaseExport();
			void OpenResponse(int response);

			void LogMapped();
			void SolutionMapped();

			void UpdateIds();
	};

	template <typename T>
	inline Glib::RefPtr<T>
	Window::Get(std::string const &name)
	{
		return Glib::RefPtr<T>::cast_dynamic(d_builder->get_object(name));
	}
}

#endif /* __OPTIEXTRACTOR_WINDOW_H__ */

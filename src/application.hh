#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <gtkmm.h>
#include <jessevdk/db/db.hh>
#include <map>
#include <string>

#include "runner.hh"

class Application : public Gtk::Window
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

	std::map<std::string, std::string> d_dispatchers;
	bool d_scanned;
	
	public:
		/* Constructor/destructor */
		Application();

		/* Public functions */
		Gtk::Window &Window();
		
		void Open(std::string const &filename);
		
		template <typename T>
		Glib::RefPtr<T> Get(std::string const &name);
	private:
		/* Private functions */
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
		
		std::string ResolveDispatcher();
		
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
};

template <typename T>
inline Glib::RefPtr<T>
Application::Get(std::string const &name)
{
	return Glib::RefPtr<T>::cast_dynamic(d_builder->get_object(name));
}

#endif /* __APPLICATION_H__ */

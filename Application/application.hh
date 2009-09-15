#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <gtkmm.h>
#include <db/db.hh>
#include <map>
#include <string>

#include "Runner/runner.hh"

class Application : public Gtk::Window
{
	db::SQLite d_database;
	Glib::RefPtr<Gtk::Builder> d_builder;
	Gtk::Window *d_window;
	Gtk::MessageDialog *d_dialog;
	std::map<std::string, std::string> d_parameterMap;
	
	Glib::RefPtr<Gtk::UIManager> d_uiManager;
	Glib::RefPtr<Gtk::ActionGroup> d_actionGroup;
	
	Gtk::FileChooserDialog *d_openDialog;
	
	Runner d_runner;
	optimization::messages::worker::Response d_lastResponse;
	std::string d_overrideDispatcher;

	std::map<std::string, std::string> d_dispatchers;
	bool d_scanned;
	
	public:
		/* Constructor/destructor */
		Application();

		/* Public functions */
		Gtk::Window &window();
		
		void open(std::string const &filename);
		
		template <typename T>
		Glib::RefPtr<T> get(std::string const &name);
	private:
		/* Private functions */
		void initializeUI();
		void error(std::string const &error, std::string const &secondary = "");
	
		void fill();
		
		void fillKeyValue(std::string const &storeName, std::string const &table, std::string const &keyname, std::string const &valuename, std::string const &condition = "");

		void fillKeyValue(Glib::RefPtr<Gtk::ListStore> store, std::string const &table, std::string const &keyname, std::string const &valuename, std::string const &condition = "");
		
		void fillBoundaries();
		void fillLog();

		void clear();
		
		void destroyDialog(int response);
		void solutionChanged();
		void executeClicked();
		
		void runnerState(bool running);
		void runnerResponse(optimization::messages::worker::Response const &response);
		
		void runSolution();
		void handleRunnerStarted();
		void handleRunnerStopped();
		
		std::string resolveDispatcher();
		
		void scan();
		void scan(std::string const &directory);
		
		void overrideAdd();
		void overrideRemove();
		
		void overrideNameEdited(Glib::ustring const &path, Glib::ustring const &newtext);
		void overrideValueEdited(Glib::ustring const &path, Glib::ustring const &newtext);
		
		std::string formatDate(size_t timestamp, std::string const &format = "%d-%m, %R") const;
		
		void onFileOpen();
		void onFileExport();
		void onFileClose();
		void onFileQuit();
		
		void openResponse(int response);
};

template <typename T>
inline Glib::RefPtr<T> Application::get(std::string const &name)
{
	return Glib::RefPtr<T>::cast_dynamic(d_builder->get_object(name));
}

#endif /* __APPLICATION_H__ */

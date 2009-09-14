#include "application.ih"

void Application::fillKeyValue(string const &storeName, string const &table, string const &keyname, string const &valuename, string const &condition)
{
	fillKeyValue(get<Gtk::ListStore>(storeName), table, keyname, valuename, condition);
}

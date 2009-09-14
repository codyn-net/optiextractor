#include "application.ih"

void Application::overrideNameEdited(Glib::ustring const &path, Glib::ustring const &newtext)
{
	Glib::RefPtr<Gtk::ListStore> store = get<Gtk::ListStore>("list_store_override");

	Gtk::TreeModel::iterator iter = store->get_iter(path);
	Gtk::TreeRow row = *iter;
	
	row->set_value(0, newtext);
}

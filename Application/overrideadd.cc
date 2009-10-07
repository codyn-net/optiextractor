#include "application.ih"

void Application::overrideAdd()
{
	Glib::RefPtr<Gtk::ListStore> store = get<Gtk::ListStore>("list_store_override");
	Glib::RefPtr<Gtk::TreeView> tv = get<Gtk::TreeView>("tree_view_override");
	
	Gtk::TreeIter iter = store->append();
	Gtk::TreeRow row = *iter;
	
	row.set_value(0, string("key"));
	row.set_value(1, string("value"));
	
	Gtk::TreePath path = store->get_path(iter);
	
	tv->scroll_to_cell(path, *tv->get_column(0), 0.5, 0.5);
	tv->set_cursor(path, *tv->get_column(0), true);
	
	d_database.query("DELETE FROM `optiextractor_overrides` WHERE `name` = 'key'");
	d_database.query("INSERT INTO `optiextractor_overrides` (`name`, `value`) VALUES ('key', 'value')");
}

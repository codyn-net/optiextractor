#include "application.ih"

void Application::fillOverrides()
{
	Glib::RefPtr<Gtk::ListStore> store = get<Gtk::ListStore>("list_store_override");
	
	store->clear();
	Row row = d_database.query("SELECT `name`, `value` FROM optiextractor_overrides");
	
	if (row.done())
	{
		return;
	}
	
	while (row)
	{
		Gtk::TreeRow r = *(store->append());
		
		r->set_value(0, row.get<string>(0));
		r->set_value(1, row.get<string>(1));

		row.next();
	}
}

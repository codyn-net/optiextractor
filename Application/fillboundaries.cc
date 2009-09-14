#include "application.ih"

void Application::fillBoundaries()
{
	Glib::RefPtr<Gtk::ListStore> store = get<Gtk::ListStore>("list_store_boundaries");
	
	store->clear();
	
	Row row = d_database.query("SELECT `name`, `min`, `max` FROM boundaries ORDER BY `name`");
	
	if (row.done())
	{
		return;
	}
	
	while (row)
	{
		Gtk::TreeRow iter = *(store->append());
		
		iter.set_value(0, row.get<string>(0));
		iter.set_value(1, row.get<string>(1));
		iter.set_value(2, row.get<string>(2));
		
		row.next();
	}
}

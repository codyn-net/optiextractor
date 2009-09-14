#include "application.ih"

void Application::fillLog()
{
	Glib::RefPtr<Gtk::ListStore> store = get<Gtk::ListStore>("list_store_log");
	
	store->clear();
	Row row = d_database.query("SELECT * FROM log");
	
	if (row.done())
	{
		return;
	}
	
	while (row)
	{
		Gtk::TreeRow r = *(store->append());
		
		r->set_value(0, formatDate(row.get<size_t>(0)));
		r->set_value(1, row.get<string>(1));
		r->set_value(2, row.get<string>(2));

		row.next();
	}
}

#include "application.ih"

void Application::fillKeyValue(Glib::RefPtr<Gtk::ListStore> store, string const &table, string const &keyname, string const &valuename, string const &condition)
{
	store->clear();
	
	stringstream q;
	
	q << "SELECT `" << keyname << "`, `" << valuename << "` FROM `" << table << "`";
	
	if (condition != "")
	{
		q << " WHERE " << condition;
	}
	
	q << " ORDER BY `" << keyname << "`";
	Row row = d_database.query(q.str());
	
	if (row.done())
	{
		return;
	}
	
	while (row)
	{
		Gtk::TreeRow iter = *(store->append());
		
		iter.set_value(0, row.get<string>(0));
		iter.set_value(1, row.get<string>(1));
		
		row.next();
	}
}

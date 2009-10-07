#include "application.ih"

void Application::overrideRemove()
{
	Glib::RefPtr<Gtk::ListStore> store = get<Gtk::ListStore>("list_store_override");
	Glib::RefPtr<Gtk::TreeView> tv = get<Gtk::TreeView>("tree_view_override");
	
	Gtk::TreeModel::iterator iter = tv->get_selection()->get_selected();
	
	if (!iter)
	{
		return;
	}
	
	string name;
	iter->get_value(0, name);
	
	d_database.query() << "DELETE FROM optiextractor_overrides WHERE `name` = '" 
	                   << SQLite::serialize(name) << "'"
	                   << SQLite::Query::End();
	
	iter = store->erase(iter);
		
	if (iter)
	{
		tv->get_selection()->select(iter);
	}
	else
	{
		size_t num = store->children().size();
		
		if (num != 0)
		{
			stringstream s;
			s << (num - 1);
			
			tv->get_selection()->select(Gtk::TreePath(s.str()));
		}
	}
}

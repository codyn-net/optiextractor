#include "application.ih"

void Application::fill()
{
	if (!d_database)
	{
		return;
	}

	get<Gtk::Notebook>("notebook")->set_sensitive(true);
	
	get<Gtk::Label>("label_summary_optimizer")->set_text(d_database.query("SELECT `value` FROM  settings WHERE `name` = 'optimizer'").get<string>(0));

	/* Best fitness */
	get<Gtk::Label>("label_summary_best")->set_text(d_database.query("SELECT MAX(`best_fitness`) FROM `iteration`").get<string>(0));
	
	time_t first = d_database.query("SELECT `time` FROM `iteration` WHERE `iteration` = 0").get<size_t>(0);
	time_t last = d_database.query("SELECT `time` FROM `iteration` ORDER BY `time` DESC LIMIT 1").get<size_t>(0);
	
	get<Gtk::Label>("label_summary_time")->set_text(formatDate(first) + "   to   "  + formatDate(last));

	fillKeyValue("list_store_settings", "settings", "name", "value", "`name` <> 'optimizer'");
	fillKeyValue("list_store_fitness", "fitness_settings", "name", "value");
	fillKeyValue("list_store_dispatcher", "dispatcher", "name", "value");
	
	fillKeyValue("list_store_parameters", "parameters", "name", "boundary");
	
	Gtk::TreeModel::iterator iter;
	Glib::RefPtr<Gtk::ListStore> store = get<Gtk::ListStore>("list_store_parameters");\

	for (iter = store->children().begin(); iter != store->children().end(); ++iter)
	{
		Gtk::TreeRow r = *iter;
		string name;
		string value;
		
		r.get_value(0, name);
		r.get_value(1, name);
		
		d_parameterMap[name] = value;
	}
	
	fillBoundaries();
	fillLog();
	
	size_t maxiteration = d_database.query("SELECT MAX(`iteration`) FROM `solution`").get<size_t>(0);
	size_t maxindex = d_database.query("SELECT MAX(`index`) FROM `solution`").get<size_t>(0);
	
	get<Gtk::Range>("hscale_iteration")->set_range(0, maxiteration + 1);
	get<Gtk::Range>("hscale_solution")->set_range(0, maxindex + 1);
	
	solutionChanged();
}

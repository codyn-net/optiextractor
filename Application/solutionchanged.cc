#include "application.ih"

void Application::solutionChanged()
{
	get<Gtk::Label>("label_solution_fitness")->set_text("");
	get<Gtk::Label>("label_solution_iteration")->set_text("");
	get<Gtk::Label>("label_solution_solution")->set_text("");
	
	Glib::RefPtr<Gtk::ListStore> store = get<Gtk::ListStore>("list_store_solutions");
	store->clear();
	
	if (!get<Gtk::Notebook>("notebook")->sensitive())
	{
		return;
	}
	
	size_t iteration = get<Gtk::Range>("hscale_iteration")->get_value();
	size_t solution = get<Gtk::Range>("hscale_solution")->get_value();
	
	stringstream q;
	
	q << "SELECT `iteration`, `index`, `fitness`, `values`, `value_names` FROM solution";
	
	if (solution == 0 && iteration != 0)
	{
		q << " WHERE `iteration` = " << (iteration - 1);
	}
	else if (iteration == 0 && solution != 0)
	{
		q << " WHERE `index` = " << (solution - 1);
	}
	else if (iteration != 0 && solution != 0)
	{
		q << " WHERE `iteration` = " << (iteration - 1) << " AND `index` = " << (solution - 1);
	}
	
	q << " ORDER BY `fitness` DESC LIMIT 1";
	
	Row row = d_database.query(q.str());
	
	if (row.done())
	{
		return;
	}
	
	{
		stringstream s;
		s << (row.get<size_t>(0) + 1);
		get<Gtk::Label>("label_solution_iteration")->set_text(s.str());
	}

	{
		stringstream s;
		s << (row.get<size_t>(1) + 1);
		get<Gtk::Label>("label_solution_solution")->set_text(s.str());
	}
	
	get<Gtk::Label>("label_solution_fitness")->set_text(row.get<string>(2));
	
	vector<string> values = String(row.get<string>(3)).split(",");
	vector<string> names = String(row.get<string>(4)).split(",");
	
	for (size_t i = 0; i < values.size(); ++i)
	{
		Gtk::TreeRow row = *(store->append());
		
		row.set_value(0, string(String(names[i]).strip()));
		row.set_value(1, string(String(values[i]).strip()));
	}
}

#include "application.ih"

void Application::onDatabaseExport()
{
	Gtk::FileChooserDialog *dialog;
	
	dialog = new Gtk::FileChooserDialog(window(), 
	                                   "Export Optimization Database",
	                                    Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog->add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

	dialog->set_local_only(true);
	
	Gtk::FileFilter *filter = new Gtk::FileFilter();

	filter->set_name("Matlab data text files (*.txt)");
	filter->add_pattern("*.txt");
	dialog->add_filter(*filter);
	delete filter;
	
	filter = new Gtk::FileFilter();
	filter->set_name("All files (*)");
	filter->add_pattern("*");
	dialog->add_filter(*filter);
	delete filter;

	if (dialog->run() != Gtk::RESPONSE_OK)
	{
		delete dialog;
	
		return;
	}
	
	string filename = dialog->get_filename();
	delete dialog;

	ofstream fstr(filename.c_str(), ios::out);
	
	if (!fstr)
	{
		error("Could not create file to export to");
		return;
	}
	
	size_t iterations = d_database.query("SELECT COUNT(*) FROM `solution` GROUP BY `index`").get<size_t>(0);
	size_t solutions = d_database.query("SELECT COUNT(*) FROM `solution`").get<size_t>(0);
	
	/* Collect fitness names */
	Row cols = d_database.query("PRAGMA table_info(`fitness`)");
	vector<string> fitnesses;
	vector<string> colnames;
	
	fitnesses.push_back("value");
	colnames.push_back("`fitness`.value");
	
	if (!cols.done())
	{
		while (cols)
		{
			String name = cols.get<string>(1);
			
			if (name.startsWith("_"))
			{
				fitnesses.push_back(name);
				colnames.push_back("`fitness`." + name);
			}
			
			cols.next();
		}
	}
	
	if (fitnesses.size() == 2)
	{
		fitnesses.erase(fitnesses.begin() + 1);
		colnames.erase(colnames.begin() + 1);
	}
	
	vector<string> boundaries;
	
	Glib::RefPtr<Gtk::ListStore> store = get<Gtk::ListStore>("list_store_boundaries");
	Gtk::TreeModel::iterator iter;

	for (iter = store->children().begin(); iter != store->children().end(); ++iter)
	{
		for (size_t i = 0; i < 3; ++i)
		{
			string value;
			(*iter)->get_value(i, value);
			boundaries.push_back(value);
		}
	}
	
	std::map<std::string, std::string>::iterator it;
	vector<string> parameters;
	
	for (it = d_parameterMap.begin(); it != d_parameterMap.end(); ++it)
	{
		parameters.push_back(it->first);
		parameters.push_back(it->second);
	}
	
	size_t totalnum = iterations * solutions;
	string names = String::join(colnames, ", ");
	
	Row row = d_database.query("SELECT solution.`iteration`, solution.`index`, `values`, `velocity`, `value_names`, " + names + " FROM `solution` LEFT JOIN `fitness` ON (fitness.iteration = solution.iteration AND fitness.`index` = solution.`index`) ORDER BY solution.`iteration`, solution.`index`");
	
	bool header = false;
	
	if (!row.done())
	{
		while (row)
		{
			if (!header)
			{
				size_t num = String(row.get<string>(2)).split(",").size();
				string nm = String::join(String(row.get<string>(4)).split(","), "\t");
				string fitnm = String::join(fitnesses, "\t");
				
				fstr << num << "\t" << iterations << "\t" << solutions << "\t" << nm << "\t" << fitnm << endl;
				fstr << String::join(boundaries, "\t") << endl;
				fstr << String::join(parameters, "\t") << endl;
				
				header = true;
			}
			
			fstr << row.get<size_t>(0) << "\t" << row.get<size_t>(1);
			
			for (size_t i = 5; i < 5 + fitnesses.size(); ++i)
			{
				fstr << row.get<string>(i);
			}
			
			fstr << String::join(String(row.get<string>(2)).split(","), "\t");
			fstr << String::join(String(row.get<string>(3)).split(","), "\t");
			fstr << endl;

			row.next();
		}
	}
	
	fstr.close();
}

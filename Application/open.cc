#include "application.ih"

void Application::open(string const &filename)
{
	clear();
	
	if (!FileSystem::fileExists(filename))
	{
		error("<b>Database file does not exist</b>", "The file '<i>" + filename + "</i>' does not exist. Please make sure you open a valid optimization results database.");
		return;
	}
	
	d_database = SQLite(filename);
	
	if (!d_database)
	{
		error("<b>Database could not be opened</b>", "The file '<i>" + filename + "</i>' could not be opened. Please make sure the file is a valid optimization results database.");
		return;
	}

	/* do a test query */
	Row row = d_database.query("PRAGMA table_info(settings)");

	if (!d_database.query("PRAGMA quick_check") || (!row || row.done()))
	{
		error("<b>Database could not be opened</b>", "The file '<i>" + filename + "</i>' could not be opened. Please make sure the file is a valid optimization results database.");
		return;
	}
	
	/* Do something here */
	fill();
}

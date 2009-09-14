#include "application.ih"

void Application::open(string const &filename)
{
	clear();
	
	if (!FileSystem::fileExists(filename))
	{
		error("Database file does not exist: " + filename);
		return;
	}
	
	d_database = SQLite(filename);
	
	if (!d_database)
	{
		error("Database could not be opened: " + filename);
		return;
	}
	
	/* Do something here */
	fill();
}

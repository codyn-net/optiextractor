#include "application.ih"

string Application::resolveDispatcher()
{
	// First try to find the dispatcher from the database
	Row row = d_database.query("SELECT `value` FROM `dispatcher` WHERE `name` = 'dispatcher'");
	
	if (row.done())
	{
		// No dispatcher found
		return "";
	}
	
	string dispatcher = row.get<string>(0);

	/* Try to locate dispatcher */
	if (FileSystem::isAbsolute(dispatcher))
	{
		return FileSystem::fileExists(dispatcher) ? dispatcher : "";
	}
	else
	{
		scan();

		std::map<std::string, std::string>::iterator fnd = d_dispatchers.find(dispatcher);
		dispatcher = (fnd != d_dispatchers.end()) ? fnd->second : "";
	}
	
	return dispatcher;
}

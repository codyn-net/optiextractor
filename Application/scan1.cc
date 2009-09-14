#include "application.ih"

void Application::scan()
{
	if (d_scanned)
	{
		return;
	}
	
	vector<string> paths = Environment::path("OPTIMIZATION_DISPATCHERS_PATH");
	
	for (vector<string>::iterator iter = paths.begin(); iter != paths.end(); ++iter)
	{
		scan(*iter);
	}
	
	string path;

	if (FileSystem::realpath(PREFIXDIR "/libexec/liboptimization-dispatchers-0.1", path))
	{
		scan(path);
	}
	
	d_scanned = true;	
}

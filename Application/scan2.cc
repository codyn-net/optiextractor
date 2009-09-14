#include "application.ih"

void Application::scan(string const &directory)
{
	if (!FileSystem::directoryExists(directory))
	{
		return;
	}
	
	DIR *d = opendir(directory.c_str());
	
	if (!d)
	{
		return;
	}
	
	struct dirent *dent;
	
	while ((dent = readdir(d)))
	{
		String s(Glib::build_filename(directory, dent->d_name));
		struct stat buf;

		if (stat(s.c_str(), &buf) != 0)
		{
			continue;
		}

		if (!S_ISREG(buf.st_mode) && !S_ISLNK(buf.st_mode))
		{
			continue;
		}
		
		if (!(buf.st_mode & S_IXUSR))
		{
			continue;
		}

		d_dispatchers[dent->d_name] = s;
	}
	
	closedir(d);	
}

#include "dispatcher.hh"
#include <sys/types.h>
#include <dirent.h>
#include <jessevdk/os/os.hh>
#include <jessevdk/base/string.hh>

using namespace optiextractor;
using namespace jessevdk::os;
using namespace jessevdk::base;
using namespace std;

Dispatcher *Dispatcher::s_instance = 0;

Dispatcher &
Dispatcher::Instance()
{
	if (!s_instance)
	{
		s_instance = new Dispatcher();
	}

	return *s_instance;
}

Dispatcher::Dispatcher()
:
	d_scanned(false)
{
}

string
Dispatcher::Resolve(string const &dispatcher)
{
	/* Try to locate dispatcher */
	if (FileSystem::IsAbsolute(dispatcher))
	{
		return FileSystem::FileExists(dispatcher) ? dispatcher : "";
	}
	else
	{
		Dispatcher &instance = Instance();

		instance.Scan();

		std::map<std::string, std::string>::iterator fnd = instance.d_dispatchers.find(dispatcher);
		return (fnd != instance.d_dispatchers.end()) ? fnd->second : "";
	}
}

void
Dispatcher::Scan()
{
	if (d_scanned)
	{
		return;
	}

	vector<string> paths = Environment::Path("OPTIMIZATION_DISPATCHERS_PATH");

	for (vector<string>::iterator iter = paths.begin(); iter != paths.end(); ++iter)
	{
		Scan(*iter);
	}

	string path;

	if (FileSystem::Realpath(PREFIXDIR "/libexec/liboptimization-dispatchers-2.0", path))
	{
		Scan(path);
	}

	d_scanned = true;
}

void
Dispatcher::Scan(string const &directory)
{
	if (!FileSystem::DirectoryExists(directory))
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


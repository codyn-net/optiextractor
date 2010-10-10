#include "utils.hh"

#include <jessevdk/base/string.hh>

using namespace std;
using namespace jessevdk::db;
using namespace optiextractor;
using namespace jessevdk::base;

vector<string>
Utils::FilterActive(sqlite::SQLite database, size_t iteration, size_t solution, vector<string> const &ret)
{
	sqlite::Row hasactive = database("SELECT COUNT(*) FROM `parameter_active`");

	if (!hasactive || hasactive.Done())
	{
		return ret;
	}

	sqlite::Row active = database() << "SELECT `" << String::Join(ret, "`, `") << "` "
	                                << "FROM parameter_active WHERE "
	                                << "`iteration` = " << iteration << " AND "
	                                << "`index` = " << solution
	                                << sqlite::SQLite::Query::End();

	vector<string> filtered;

	for (size_t i = 0; i < ret.size(); ++i)
	{
		if (active.Get<int>(i))
		{
			filtered.push_back(ret[i]);
		}
	}

	return filtered;
}

vector<string>
Utils::ActiveParameters(sqlite::SQLite database, size_t iteration, size_t solution)
{
	sqlite::Row names = database("PRAGMA table_info(`parameter_values`)");

	vector<string> ret;

	while (names && !names.Done())
	{
		string name = names.Get<string>(1);
		names.Next();

		if (!String(name).StartsWith("_p_"))
		{
			continue;
		}

		ret.push_back(name);
	}

	ret = Utils::FilterActive(database, iteration, solution, ret);

	return ret;
}

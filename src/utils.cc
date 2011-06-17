#include "utils.hh"

#include <jessevdk/base/string.hh>

using namespace std;
using namespace jessevdk::db;
using namespace optiextractor;
using namespace jessevdk::base;

vector<string>
Utils::FilterActive(sqlite::SQLite database, size_t iteration, size_t solution, vector<string> const &ret)
{
	sqlite::Row hasactive = database("PRAGMA table_info(parameter_active)");

	if (!hasactive || hasactive.Done())
	{
		return ret;
	}

	hasactive = database("SELECT COUNT(*) FROM `parameter_active`");

	if (!hasactive || hasactive.Done())
	{
		return ret;
	}

	sqlite::Row active = database() << "SELECT `" << String::Join(ret, "`, `") << "` "
	                                << "FROM parameter_active WHERE "
	                                << "`iteration` = " << iteration << " AND "
	                                << "`index` = " << solution
	                                << sqlite::SQLite::Query::End();

	if (!active || active.Done())
	{
		return ret;
	}

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

vector<string>
Utils::DataColumns(sqlite::SQLite database)
{
	sqlite::Row names = database("PRAGMA table_info(`data`)");

	vector<string> ret;

	while (names && !names.Done())
	{
		string name = names.Get<string>(1);
		names.Next();

		if (!String(name).StartsWith("_d_") || String(name).StartsWith("_d_velocity_"))
		{
			continue;
		}

		ret.push_back(name);
	}

	return ret;
}

bool
Utils::FindBest(sqlite::SQLite  database,
                int             iteration,
                int             solution,
                size_t         &iterationOut,
                size_t         &solutionOut)
{
	bool iterationBest = iteration < 0;
	bool solutionBest = solution < 0;

	if (!iterationBest && !solutionBest)
	{
		iterationOut = iteration;
		solutionOut = solution;

		return true;
	}

	if (IsStagePSO(database))
	{
		return FindStagePSOBest(database, iteration, solution, iterationOut, solutionOut);
	}

	if (solutionBest && !iterationBest)
	{
		sqlite::Row res = database() << "SELECT `index` FROM solution WHERE `iteration` = "
		                             << iteration << " ORDER BY `fitness` DESC LIMIT 1"
		                             << sqlite::SQLite::Query::End();

		if (!res || res.Done())
		{
			return false;
		}

		iterationOut = iteration;
		solutionOut = res.Get<int>(0);
	}
	else if (iterationBest && !solutionBest)
	{
		sqlite::Row res = database() << "SELECT `iteration` FROM solution WHERE `index` = "
		                             << solution << " ORDER BY `fitness` DESC LIMIT 1"
		                             << sqlite::SQLite::Query::End();

		if (!res || res.Done())
		{
			return false;
		}

		iterationOut = res.Get<int>(0);
		solutionOut = solution;
	}
	else if (iterationBest && solutionBest)
	{
		sqlite::Row res = database() << "SELECT `iteration`, `index` FROM solution "
		                             << "ORDER BY `fitness` DESC LIMIT 1"
		                             << sqlite::SQLite::Query::End();

		if (!res || res.Done())
		{
			return false;
		}

		iterationOut = res.Get<int>(0);
		solutionOut = res.Get<int>(1);
	}

	return true;
}

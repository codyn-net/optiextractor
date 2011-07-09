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
Utils::Columns(sqlite::SQLite  database,
               string const   &table,
               string const   &prefix)
{
	sqlite::Row names = database("PRAGMA table_info(`" + table + "`)");

	vector<string> ret;

	while (names && !names.Done())
	{
		String nm = names.Get<string>(1);

		if (prefix == "" || nm.StartsWith(prefix))
		{
			ret.push_back(nm);
		}

		names.Next();
	}

	return ret;
}

vector<string>
Utils::DataColumns(sqlite::SQLite database)
{
	vector<string> names = Utils::Columns(database, "data");
	vector<string> ret;

	for (vector<string>::iterator iter = names.begin(); iter != names.end(); ++iter)
	{
		if (!String(*iter).StartsWith("_d_") || String(*iter).StartsWith("_d_velocity_"))
		{
			continue;
		}

		ret.push_back(*iter);
	}

	return ret;
}

bool
Utils::IsStagePSO(sqlite::SQLite database)
{
	sqlite::Row res = database("PRAGMA table_info(stagepso_settings)");

	return res && !res.Done();
}

bool
Utils::FindStagePSOBest(sqlite::SQLite  database,
                        int             iteration,
                        int             solution,
                        size_t         &iterationOut,
                        size_t         &solutionOut)
{
	bool iterationBest = iteration < 0;
	bool solutionBest = solution < 0;

	if (solutionBest && !iterationBest)
	{
		sqlite::Row res =
			database() << "SELECT "
			                  "solution.`index` "
			              "FROM "
			                  "solution "
			              "LEFT JOIN "
			                  "data "
			              "ON "
			                  "(solution.iteration = data.iteration AND "
			                  " solution.`index` = data.`index`) "
			              "WHERE "
			                  "solution.iteration = " << iteration << " "
			           << "ORDER BY "
			                  "data.`_d_StagePSO::stage` DESC, "
			                  "solution.fitness DESC "
			              "LIMIT 1"
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
		sqlite::Row res =
			database() << "SELECT "
			                  "solution.iteration "
			              "FROM "
			                  "solution "
			              "LEFT JOIN "
			                  "data "
			              "ON "
			                  "(solution.iteration = data.iteration AND "
			                  " solution.`index` = data.`index`) "
			              "WHERE "
			                  "solution.`index` = " << solution << " "
			           << "ORDER BY "
			                  "data.`_d_StagePSO::stage` DESC, "
			                  "solution.fitness DESC "
			              "LIMIT 1"
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
		sqlite::Row res =
			database() << "SELECT "
			                  "data.iteration, "
			                  "data.`index` "
			              "FROM "
			                  "data "
			              "LEFT JOIN "
			                  "fitness "
			              "ON "
			                  "(fitness.iteration = data.iteration AND "
			                  " fitness.`index` = data.`index`) "
			              "WHERE "
			                  "data.`_d_StagePSO::stage` = (SELECT MAX(`_d_StagePSO::stage`) FROM data) "
			              "ORDER BY "
			                  "fitness.value DESC "
			              "LIMIT 1"
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

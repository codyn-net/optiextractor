#ifndef __OPTIEXTRACTOR_UTILS_H__
#define __OPTIEXTRACTOR_UTILS_H__

#include <jessevdk/db/db.hh>
#include <vector>
#include <string>

namespace optiextractor
{
	class Utils
	{
		public:
			static std::vector<std::string>
			ActiveParameters(jessevdk::db::sqlite::SQLite database,
			                 size_t                       iteration,
			                 size_t                       solution);

			static std::vector<std::string>
			DataColumns(jessevdk::db::sqlite::SQLite database);

			static bool
			FindBest(jessevdk::db::sqlite::SQLite  database,
			         int                           iteration,
			         int                           solution,
			         size_t                       &iterationOut,
			         size_t                       &solutionOut);
		private:
			static std::vector<std::string>
			FilterActive(jessevdk::db::sqlite::SQLite    database,
			             size_t                          iteration,
			             size_t                          solution,
			             std::vector<std::string> const &ret);

			static bool
			FindStagePSOBest(jessevdk::db::sqlite::SQLite  database,
			                 int                           iteration,
			                 int                           solution,
			                 size_t                       &iterationOut,
			                 size_t                       &solutionOut);

			static bool
			IsStagePSO(jessevdk::db::sqlite::SQLite database);
	};
}

#endif /* __OPTIEXTRACTOR_UTILS_H__ */


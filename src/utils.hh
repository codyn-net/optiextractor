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
			static std::vector<std::string> ActiveParameters(jessevdk::db::sqlite::SQLite database, size_t iteration, size_t solution);
		private:
			static std::vector<std::string> FilterActive(jessevdk::db::sqlite::SQLite database, size_t iteration, size_t solution, std::vector<std::string> const &ret);
	};
}

#endif /* __OPTIEXTRACTOR_UTILS_H__ */

#ifndef __OPTIEXTRACTOR_DISPATCHER_H__
#define __OPTIEXTRACTOR_DISPATCHER_H__

#include <map>
#include <string>

namespace optiextractor
{
	class Dispatcher
	{
		static Dispatcher *s_instance;
		std::map<std::string, std::string> d_dispatchers;
		bool d_scanned;

		public:
			static Dispatcher &Instance();

			static std::string Resolve(std::string const &dispatcher);
		private:
			Dispatcher();

			void Scan();
			void Scan(std::string const &directory);
	};
}

#endif /* __OPTIEXTRACTOR_DISPATCHER_H__ */

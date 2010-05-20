#ifndef __OPTIEXTRACTOR_EXPORTER_H__
#define __OPTIEXTRACTOR_EXPORTER_H__

#include <ostream>
#include <jessevdk/db/sqlite.hh>
#include <sstream>
#include <vector>

namespace optiextractor
{
	class Exporter
	{
		std::ostream &d_stream;
		jessevdk::db::sqlite::SQLite &d_database;
		size_t d_level;
		std::string d_indentation;

		public:
			Exporter(std::ostream &ostr, jessevdk::db::sqlite::SQLite &database);

			void Export();
		private:
			/* Private functions */
			void ExportBoundaries();
			void ExportDispatcherSettings();
			void ExportFitnessSettings();
			void ExportJob();
			void ExportOptimizerSettings();
			void ExportParameters();
			void ExportSolutions();

			void Begin(std::string const &name = "");
			void End();

			std::string Indentation() const;

			template <typename T>
			void Write(std::string const &name, T const &t);

			template <typename T>
			std::string Serialize(std::vector<T> const &v) const;

			std::string Serialize(std::string const &v) const;
			std::string Serialize(double v) const;
			std::string Serialize(size_t v) const;
			std::string Serialize(int v) const;

			template <typename T>
			std::string SerializeStream(T const &v) const;

			std::string NormalizeName(std::string const &name) const;
			void WriteNames(std::string const &name, jessevdk::db::sqlite::Row row, std::string const &prefix);

			void ExportIterations();
	};

	template <typename T>
	void
	Exporter::Write(std::string const &name, T const &t)
	{
		d_stream << Indentation() << NormalizeName(name) << ": " << Serialize(t) << std::endl;
	}

	template <typename T>
	std::string
	Exporter::Serialize(std::vector<T> const &v) const
	{
		std::stringstream s;

		for (typename std::vector<T>::const_iterator iter = v.begin(); iter != v.end(); ++iter)
		{
			if (s.str() != "")
			{
				s << ", ";
			}

			s << Serialize(*iter);
		}

		return "[" + s.str() + "]";
	}

	template <typename T>
	std::string
	Exporter::SerializeStream(T const &v) const
	{
		std::stringstream s;

		s << v;
		return s.str();
	}
}

#endif /* __OPTIEXTRACTOR_EXPORTER_H__ */

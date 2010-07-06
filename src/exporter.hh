#ifndef __OPTIEXTRACTOR_EXPORTER_H__
#define __OPTIEXTRACTOR_EXPORTER_H__

#include <ostream>
#include <jessevdk/db/sqlite.hh>
#include <sstream>
#include <vector>
#include <matio.h>
#include <stack>

namespace optiextractor
{
	class Exporter
	{
		std::string d_filename;
		jessevdk::db::sqlite::SQLite &d_database;
		std::stack<matvar_t *> d_structures;
		mat_t *d_matlab;

		public:
			Exporter(std::string const &filename, jessevdk::db::sqlite::SQLite &database);

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

			template <typename T>
			void Write(std::string const &name, T const &t);

			matvar_t *Serialize(std::string const &nm, std::string const &v) const;
			matvar_t *Serialize(std::string const &nm, double v) const;
			matvar_t *Serialize(std::string const &nm, size_t v) const;
			matvar_t *Serialize(std::string const &nm, int v) const;

			std::string NormalizeName(std::string const &name) const;

			void WriteNames(std::string const &name, jessevdk::db::sqlite::Row row, std::string const &prefix, std::string const &additional = "");

			void ExportIterations();

			void Write(matvar_t *var);
	};

	template <typename T>
	void
	Exporter::Write(std::string const &name, T const &t)
	{
		Write(Serialize(NormalizeName(name), t));
	}
}

#endif /* __OPTIEXTRACTOR_EXPORTER_H__ */

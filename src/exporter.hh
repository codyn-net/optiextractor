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
		bool d_isSystematic;
		size_t d_total;
		size_t d_ticker;

		public:
			Exporter(std::string const &filename, jessevdk::db::sqlite::SQLite &database);

			void Export();

			jessevdk::base::signals::Signal<double> OnProgress;
		private:
			/* Private functions */
			void ExportBoundaries();
			void ExportDispatcherSettings();
			void ExportFitnessSettings();
			void ExportJob();
			void ExportOptimizerSettings();
			void ExportParameters();
			void ExportSolutions();
			void ExportData();
			void ExportParameterValues();
			void ExportFitness();

			void CalculateTotalProgress();
			void EmitProgress();

			size_t Normalize3D(size_t idx, int *dims);
			size_t Normalize2D(size_t idx, int *dims);

			int *MatrixDimensions(std::string const &name,
			                      int &numdim,
			                      size_t &size);

			void ExportMatrix(std::string const &name);
			void ExportMatrix(std::string const &table, std::string const &name);

			size_t NumberOfColumns(std::string const &name);

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

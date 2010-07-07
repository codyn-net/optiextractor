#include "exporter.hh"
#include <jessevdk/base/string.hh>
#include <iomanip>
#include <glibmm.h>

using namespace optiextractor;
using namespace std;
using namespace jessevdk::db::sqlite;
using namespace jessevdk::base;

Exporter::Exporter(std::string const &filename, SQLite &database)
:
	d_filename(filename),
	d_database(database)
{
}

void
Exporter::Export()
{
	d_matlab = Mat_Create (d_filename.c_str(), NULL);

	Row row = d_database("SELECT MAX(iteration) FROM solution");

	if (row && !row.Done())
	{
		Write("iterations", row.Get<size_t>(0) + 1);
	}

	row = d_database("SELECT MAX(`index`) FROM solution");

	if (row && !row.Done())
	{
		Write("population_size", row.Get<size_t>(0) + 1);
	}

	ExportBoundaries();
	ExportDispatcherSettings();
	ExportFitnessSettings();
	ExportOptimizerSettings();
	ExportParameters();
	ExportJob();
	ExportIterations();
	ExportSolutions();

	Mat_Close (d_matlab);
}

void
Exporter::ExportBoundaries()
{
	Row row = d_database("SELECT `name`, `min`, `max`, `min_initial`, `max_initial` FROM boundaries");

	Begin("boundaries");
	{
		while (row && !row.Done())
		{
			Begin(row.Get<string>(0));
			{
				Write("min", row.Get<double>(1));
				Write("max", row.Get<double>(2));
				Write("min_initial", row.Get<double>(3));
				Write("max_initial", row.Get<double>(4));
			}
			End();

			row.Next();
		}
	}
	End();
}

void
Exporter::ExportParameters()
{
	Row row = d_database("SELECT `name`, `boundary` FROM parameters");

	Begin("parameters");
	{
		while (row && !row.Done())
		{
			Write(row.Get<string>(0), row.Get<string>(1));
			row.Next();
		}
	}
	End();
}

void
Exporter::ExportDispatcherSettings()
{
	Row row = d_database("SELECT `name`, `value` FROM dispatcher");

	Begin("dispatcher_settings");
	{
		while (row && !row.Done())
		{
			Write(row.Get<string>(0), row.Get<string>(1));
			row.Next();
		}
	}
	End();
}

void
Exporter::ExportFitnessSettings()
{
	Row row = d_database("SELECT `name`, `value` FROM fitness_settings");

	Begin("fitness_settings");
	{
		while (row && !row.Done())
		{
			string val;

			try
			{
				val = row.Get<string>(1);
			}
			catch (...)
			{
			}

			Write(row.Get<string>(0), val);
			row.Next();
		}
	}
	End();
}

void
Exporter::ExportJob()
{
	Row row = d_database("SELECT `name`, `optimizer`, `dispatcher`, `priority`, `timeout` FROM job");

	if (row && !row.Done())
	{
		Begin("job");
		{
			Write("name", row.Get<string>(0));
			Write("optimizer", row.Get<string>(1));
			Write("dispatcher", row.Get<string>(2));
			Write("priority", row.Get<double>(3));
			Write("timeout", row.Get<double>(4));
		}
		End();
	}
}

void
Exporter::ExportOptimizerSettings()
{
	Row row = d_database("SELECT `name`, `value` FROM settings");

	Begin("optimizer_settings");
	{
		while (row && !row.Done())
		{
			Write(row.Get<string>(0), row.Get<string>(1));
			row.Next();
		}
	}
	End();
}

void
Exporter::ExportIterations()
{
	Row row = d_database("SELECT `time` FROM iteration ORDER BY iteration");

	vector<size_t> times;

	while (row && !row.Done())
	{
		times.push_back(row.Get<size_t>(0));
		row.Next();
	}

	int dims[2] = {1, times.size()};
	int data[times.size()];

	for (size_t i = 0; i < times.size(); ++i)
	{
		data[i] = times[i];
	}

	Write(Mat_VarCreate ("iteration_times", MAT_C_INT32, MAT_T_INT32, 2, dims, data, 0));
}

void
Exporter::WriteNames(string const &name, jessevdk::db::sqlite::Row row, string const &prefix, string const &additional)
{
	vector<string> names;

	while (row && !row.Done())
	{
		String name = row.Get<string>(1);

		if (name.StartsWith(prefix))
		{
			names.push_back(name.substr(prefix.length()));
		}

		row.Next();
	}

	if (additional != "")
	{
		names.push_back(additional);
	}

	int dims[2] = {1, names.size()};
	matvar_t *data[names.size()];

	for (size_t i = 0; i < names.size(); ++i)
	{
		int ddims[2] = {1, names[i].size()};
		data[i] = Mat_VarCreate ("", MAT_C_CHAR, MAT_T_UTF8, 2, ddims, (void *)names[i].c_str(), 0);
	}

	Write(Mat_VarCreate (name.c_str(), MAT_C_CELL, MAT_T_CELL, 2, dims, data, 0));
}

int *
Exporter::MatrixDimensions(string const &name, int &numdim, size_t &size)
{
	bool issystematic = false;

	Row row = d_database("SELECT `optimizer` FROM job");

	if (row)
	{
		issystematic = Glib::ustring(row.Get<string>(0)).lowercase() == "systematic";
	}

	row = d_database("SELECT COUNT(DISTINCT iteration) FROM " + name);

	if (!row)
	{
		return 0;
	}

	size_t iterations = row.Get<size_t>(0);

	row = d_database("SELECT COUNT(DISTINCT `index`) FROM " + name);
	size_t solutions = row.Get<size_t>(0);

	row = d_database("SELECT * FROM " + name + " ORDER BY iteration, `index`");

	int *dims;

	if (!issystematic)
	{
		dims = new int[3];

		dims[0] = iterations;
		dims[1] = solutions;
		dims[2] = row.Length() - 2;

		numdim = 3;
		size = dims[0] * dims[1] * dims[2];
	}
	else
	{
		dims = new int[2];
		dims[0] = iterations * solutions;
		dims[1] = row.Length() - 2;

		numdim = 2;
		size = dims[0] * dims[1];
	}

	return dims;
}

size_t
Exporter::Normalize3D(size_t idx, int *dims)
{
	size_t idval = dims[1] * dims[2];

	return idx / idval +
	       (idx % idval) / dims[2] * dims[0] +
	       (idx % dims[2]) * dims[0] * dims[1];
}

void
Exporter::ExportMatrix(string const &name)
{
	int numdim;
	size_t size;
	int *dims;

	dims = MatrixDimensions(name, numdim, size);

	if (!dims)
	{
		return;
	}

	Row row = d_database("SELECT * FROM " + name + " ORDER BY iteration, `index`");

	double data[size];
	size_t ct = 0;

	while (row && !row.Done())
	{
		for (size_t i = 2; i < row.Length(); ++i)
		{
			size_t idx = numdim == 3 ? Normalize3D(ct, dims) : ct;

			data[idx] = row.Get<double>(i);

			++ct;
		}

		row.Next();
	}

	matvar_t *var = Mat_VarCreate (name.c_str(),
	                               MAT_C_DOUBLE,
	                               MAT_T_DOUBLE,
	                               numdim,
	                               dims,
	                               data,
	                               0);

	Write(var);

	delete[] dims;
}

void
Exporter::ExportParameterValues()
{
	WriteNames("parameter_names",
	           d_database("PRAGMA table_info(parameter_values)"),
	           "_p_");

	ExportMatrix("parameter_values");
}

void
Exporter::ExportFitness()
{
	// Write the fitness names
	WriteNames("fitness_names", d_database("PRAGMA table_info(fitness)"), "_f_", "value");

	ExportMatrix("fitness");
}

void
Exporter::ExportData()
{
	WriteNames("data_names", d_database("PRAGMA table_info(data)"), "_d_");

	int numdim;
	size_t size;
	int *dims;

	dims = MatrixDimensions("data", numdim, size);

	if (!dims)
	{
		return;
	}

	matvar_t *data[size];
	size_t ct = 0;

	Row row = d_database("SELECT * FROM data ORDER BY iteration, `index`");

	while (row && !row.Done())
	{
		for (size_t i = 2; i < row.Length(); ++i)
		{
			string s = row.Get<string>(i);
			int ddims[2] = {1, s.size()};

			size_t idx = numdim == 3 ? Normalize3D(ct, dims) : ct;

			data[idx] = Mat_VarCreate ("",
			                           MAT_C_CHAR,
			                           MAT_T_UTF8,
			                           2,
			                           ddims,
			                           (void *)s.c_str(),
			                           0);

			++ct;
		}

		row.Next();
	}

	Write(Mat_VarCreate ("data_values",
	                     MAT_C_CELL,
	                     MAT_T_CELL,
	                     2,
	                     dims,
	                     data,
	                     0));

	delete[] dims;
}

void
Exporter::ExportSolutions()
{
	ExportParameterValues();
	ExportFitness();
	ExportData();
}

void
Exporter::Write(matvar_t *var)
{
	if (d_structures.empty())
	{
		Mat_VarWrite (d_matlab, var, 0);
	}
	else
	{
		matvar_t *parent = d_structures.top();

		Mat_VarAddStructField (parent, &var);
	}
}

void
Exporter::Begin(string const &name)
{
	int dims[2] = {1, 1};
	matvar_t *data[] = {NULL};

	matvar_t *st = Mat_VarCreate (name.c_str(), MAT_C_STRUCT, MAT_T_STRUCT, 2, dims, data, 0);

	d_structures.push(st);
}

void
Exporter::End()
{
	matvar_t *last = d_structures.top();

	d_structures.pop();

	Write(last);
}

matvar_t *
Exporter::Serialize(string const &name, string const &v) const
{
	int dims[2] = {1, v.size()};

	return Mat_VarCreate (name.c_str(), MAT_C_CHAR, MAT_T_UTF8, 2, dims, (void *)(v.c_str()), 0);
}

matvar_t *
Exporter::Serialize(string const &name, double v) const
{
	int dims[2] = {1, 1};

	return Mat_VarCreate (name.c_str(), MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, &v, 0);
}

matvar_t *
Exporter::Serialize(string const &name, size_t v) const
{
	int dims[2] = {1, 1};

	return Mat_VarCreate (name.c_str(), MAT_C_UINT32, MAT_T_UINT32, 2, dims, &v, 0);
}

matvar_t *
Exporter::Serialize(string const &name, int v) const
{
	int dims[2] = {1, 1};

	return Mat_VarCreate (name.c_str(), MAT_C_INT32, MAT_T_INT32, 2, dims, &v, 0);
}

string
Exporter::NormalizeName(string const &name) const
{
	String ret(name);

	return ret.Replace("-", "_").Replace(" ", "_").Replace(":", "_").Replace(".", "_");
}

#include "exporter.hh"
#include <jessevdk/base/string.hh>
#include <iomanip>
#include <glibmm.h>
#include "utils.hh"

using namespace optiextractor;
using namespace std;
using namespace jessevdk::db::sqlite;
using namespace jessevdk::base;

Exporter::Exporter(std::string const &filename, SQLite &database)
:
	d_filename(filename),
	d_database(database),
	d_isSystematic(false),
	d_numericdata(false)
{
	Row row = d_database("SELECT `optimizer` FROM job");

	if (row)
	{
		d_isSystematic = Glib::ustring(SafelyGetNullString(row, 0)).lowercase() == "systematic";
	}
}

void
Exporter::CalculateTotalProgress()
{
	d_total = 0;

	Row row = d_database("SELECT COUNT(*) FROM parameter_values");
	d_total += row.Get<size_t>(0);

	row = d_database("PRAGMA table_info(parameter_active)");

	if (row && !row.Done())
	{
		row = d_database("SELECT COUNT(*) FROM parameter_active");

		if (row && !row.Done())
		{
			d_total += row.Get<size_t>(0);
		}
	}

	row = d_database("SELECT COUNT(*) FROM fitness");
	d_total += row.Get<size_t>(0);

	row = d_database("SELECT COUNT(*) FROM data");
	d_total += row.Get<size_t>(0);

	d_ticker = 0;
}

void
Exporter::EmitProgress()
{
	double val = d_total == 0 ? 0 : (double)d_ticker / (double)d_total;

	OnProgress(val);
}

void
Exporter::Export()
{
	d_matlab = Mat_Create (d_filename.c_str(), NULL);

	Row row = d_database("SELECT MAX(iteration) FROM solution");

	CalculateTotalProgress();
	EmitProgress();

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
	ExportExtensions();
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
			Begin(SafelyGetNullString(row, 0));
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
			Write(SafelyGetNullString(row, 0), SafelyGetNullString(row, 1));
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
			Write(SafelyGetNullString(row, 0), SafelyGetNullString(row, 1));
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
			string val = SafelyGetNullString(row, 1);

			Write(SafelyGetNullString(row, 0), val);
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
			Write("name", SafelyGetNullString(row, 0));
			Write("optimizer", SafelyGetNullString(row, 1));
			Write("dispatcher", SafelyGetNullString(row, 2));
			Write("priority", row.Get<double>(3));
			Write("timeout", row.Get<double>(4));

			Row ext = d_database("SELECT DISTINCT(`name`) FROM `extensions`");
			vector<string> exts;

			if (ext)
			{
				while (!ext.Done())
				{
					string e = SafelyGetNullString(ext, 0);

					if (e != "")
					{
						exts.push_back(e);
					}

					ext.Next();
				}
			}

			Write("extensions", exts);
		}
		End();
	}
}

void
Exporter::ExportGCPSO()
{
	vector<double> successes;
	vector<double> failures;
	vector<double> samplesize;

	Row row = d_database("SELECT `successes`, `failures`, `sample_size` FROM `gcpso_samplesize` ORDER BY `iteration`");

	while (row && !row.Done())
	{
		successes.push_back(row.Get<double>(0));
		failures.push_back(row.Get<double>(1));
		samplesize.push_back(row.Get<double>(2));

		row.Next();
	}

	Begin("gcpso");
	{
		Write("successes", successes);
		Write("failures", failures);
		Write("sample_size", samplesize);
	}
	End();
}

void
Exporter::ExportStagePSO()
{
	Row cnt = d_database("SELECT COUNT(*) FROM `stages`");
	int num = cnt.Get<int>(0);

	int dims[2] = {num, 1};
	matvar_t **data = new matvar_t*[num * 2 + 1];
	data[num * 2] = 0;

	Row stage = d_database("SELECT `condition`, `expression` FROM `stages`");
	int i = 0;

	while (stage && !stage.Done())
	{
		string cond = SafelyGetNullString(stage, 0);
		string expr = SafelyGetNullString(stage, 1);

		data[i++] = Serialize("condition", cond);
		data[i++] = Serialize("expression", expr);

		stage.Next();
	}

	Write(Mat_VarCreate ("stages",
	                     MAT_C_STRUCT,
	                     MAT_T_STRUCT,
	                     2,
	                     dims,
	                     data,
	                     0));

	delete[] data;
}

void
Exporter::ExportExtension(string const &name)
{
	Row row = d_database("SELECT `name`, `value` FROM `" + name + "_settings`");

	Begin(name + "_settings");
	{
		while (row && !row.Done())
		{
			Write(row.Get<string>(0), SafelyGetNullString(row, 1));
			row.Next();
		}
	}
	End();
}

void
Exporter::ExportExtensions()
{
	Row ext = d_database("SELECT lower(`name`) FROM `extensions`");

	while (ext && !ext.Done())
	{
		string e = SafelyGetNullString(ext, 0);

		if (e == "stagepso")
		{
			ExportStagePSO();
		}
		else if (e == "gcpso")
		{
			ExportGCPSO();
		}

		ExportExtension(e);

		ext.Next();
	}
}

string
Exporter::SafelyGetNullString(Row &row, size_t idx) const
{
	try
	{
		return row.Get<string>(idx);
	}
	catch (...)
	{
		return "";
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
			Write(SafelyGetNullString(row, 0), SafelyGetNullString(row, 1));
			row.Next();
		}
	}
	End();
}

void
Exporter::SetIgnoreData(string const &ignoredata)
{
	d_ignoredata = ignoredata;
}

void
Exporter::SetNumericData(bool numeric)
{
	d_numericdata = numeric;
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

	Write(Mat_VarCreate ("iteration_times",
	                     MAT_C_INT32,
	                     MAT_T_INT32,
	                     2,
	                     dims,
	                     data,
	                     0));
}

void
Exporter::WriteNames(string const &name,
                     vector<string> const &vals,
                     string const &prefix,
                     string const &additional)
{
	vector<string> names;

	if (additional != "")
	{
		names.push_back(additional);
	}

	for (vector<string>::const_iterator iter = vals.begin(); iter != vals.end(); ++iter)
	{
		if (String(*iter).StartsWith(prefix))
		{
			names.push_back(iter->substr(prefix.length()));
		}
	}

	Write(name, names);
}

matvar_t *
Exporter::Serialize(string const &name, vector<double> const &s) const
{
	int dims[2] = {1, s.size()};
	double *data = new double[s.size()];

	for (size_t i = 0; i < s.size(); ++i)
	{
		data[i] = s[i];
	}

	matvar_t *ret = Mat_VarCreate (name.c_str(),
	                               MAT_C_DOUBLE,
	                               MAT_T_DOUBLE,
	                               2,
	                               dims,
	                               data,
	                               0);

	delete[] data;
	return ret;
}

matvar_t *
Exporter::Serialize(string const &name, vector<string> const &s) const
{
	int dims[2] = {1, s.size()};
	matvar_t **data = new matvar_t*[s.size()];

	for (size_t i = 0; i < s.size(); ++i)
	{
		int ddims[2] = {1, s[i].size()};

		data[i] = Mat_VarCreate ("",
		                         MAT_C_CHAR,
		                         MAT_T_UTF8,
		                         2,
		                         ddims,
		                         (void *)s[i].c_str(), 0);
	}

	matvar_t *ret = Mat_VarCreate (name.c_str(),
	                               MAT_C_CELL,
	                               MAT_T_CELL,
	                               2,
	                               dims,
	                               data,
	                               0);

	delete[] data;
	return ret;
}

size_t
Exporter::NumberOfColumns(string const &name, string const &prefix)
{
	Row row = d_database("PRAGMA table_info(" + name + ")");
	size_t cols = 0;

	while (row && !row.Done())
	{
		if (prefix == "" || String(row.Get<string>(1)).StartsWith(prefix))
		{
			++cols;
		}

		row.Next();
	}

	return cols;
}

int *
Exporter::MatrixDimensions(string const &name, int &numdim, size_t &size, string const &prefix)
{
	if (d_isSystematic)
	{
		Row row = d_database("SELECT COUNT(*) FROM " + name);

		if (!row)
		{
			return 0;
		}

		int *dims = new int[2];

		dims[0] = row.Get<size_t>(0);
		dims[1] = NumberOfColumns(name, prefix) - (prefix != "" ? 0 : 2);

		numdim = 2;
		size = dims[0] * dims[1];

		return dims;
	}

	Row row = d_database("SELECT COUNT(DISTINCT iteration) FROM " + name);

	if (!row)
	{
		return 0;
	}

	size_t iterations = row.Get<size_t>(0);

	row = d_database("SELECT COUNT(DISTINCT `index`) FROM " + name);
	size_t solutions = row.Get<size_t>(0);

	size_t vals = NumberOfColumns(name, prefix);

	int *dims;

	dims = new int[3];

	dims[0] = iterations;
	dims[1] = solutions;
	dims[2] = vals - (prefix != "" ? 0 : 2);

	numdim = 3;
	size = dims[0] * dims[1] * dims[2];

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

size_t
Exporter::Normalize2D(size_t idx, int *dims)
{
	return (idx / dims[1]) + (idx % dims[1]) * dims[0];
}

void
Exporter::ExportMatrix(string const &table, string const &name, string const &prefix, string const &additional)
{
	int numdim;
	size_t size;
	int *dims;

	dims = MatrixDimensions(table, numdim, size, prefix);

	if (!dims)
	{
		return;
	}

	Row row(0, 0);
	vector<string> cols = Utils::Columns(d_database, table, prefix);
	string colstr;

	if (additional != "")
	{
		dims[numdim - 1] += 1;
		size = dims[0] * dims[1] * (numdim > 2 ? dims[2] : 1);

		colstr = "`" + additional + "`";
	}

	for (vector<string>::iterator iter = cols.begin(); iter != cols.end(); ++iter)
	{
		if (*iter == "iteration" || *iter == "index")
		{
			continue;
		}

		if (colstr != "")
		{
			colstr += ", ";
		}

		colstr += "`" + *iter + "`";
	}

	if (d_isSystematic)
	{
		row = d_database("SELECT " + colstr + " FROM " + table + " ORDER BY `index`");
	}
	else
	{
		row = d_database("SELECT " + colstr + " FROM " + table + " ORDER BY iteration, `index`");
	}

	double *data = new double[size];
	size_t ct = 0;
	size_t len = dims[numdim - 1];

	while (row && !row.Done())
	{
		for (size_t i = 0; i < len; ++i)
		{
			size_t idx = numdim == 3 ? Normalize3D(ct, dims) : Normalize2D(ct, dims);
			data[idx] = row.Get<double>(i);

			++ct;
		}

		++d_ticker;
		EmitProgress();

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
	delete[] data;
}

void
Exporter::ExportMatrix(string const &name)
{
	ExportMatrix(name, name);
}

void
Exporter::ExportParameterValues()
{
	WriteNames("parameter_names",
	           Utils::Columns(d_database, "parameter_values"),
	           "_p_");

	ExportMatrix("parameter_values");
}

void
Exporter::ExportParameterActive()
{
	Row row = d_database("PRAGMA table_info(parameter_active)");

	if (row && !row.Done())
	{
		ExportMatrix("parameter_active");
	}
}

void
Exporter::ExportFitness()
{
	// Write the fitness names
	WriteNames("fitness_names",
	           Utils::Columns(d_database, "fitness"),
	           "_f_",
	           "value");

	ExportMatrix("fitness", "fitness_values", "_f_", "value");
}

void
Exporter::ExportData()
{
	vector<string> names;

	names = Utils::Columns(d_database, "data", "_d_");

	if (d_ignoredata != "")
	{
		GRegex *reg = g_regex_new (d_ignoredata.c_str(),
		                           (GRegexCompileFlags)0,
		                           (GRegexMatchFlags)0,
		                           NULL);

		if (reg)
		{
			vector<string> tmp;

			for (vector<string>::iterator iter = names.begin(); iter != names.end(); ++iter)
			{
				if (!g_regex_match (reg,
				                    (*iter).substr(3).c_str(),
				                    (GRegexMatchFlags)0,
				                    NULL))
				{
					tmp.push_back(*iter);
				}
			}

			names = tmp;

			g_regex_unref (reg);
		}
	}

	vector<string> colcomb;

	for (vector<string>::iterator iter = names.begin(); iter != names.end(); ++iter)
	{
		colcomb.push_back("`" + *iter + "`");
	}

	string colstr = String::Join(colcomb, ", ");

	WriteNames("data_names", names, "_d_");

	int numdim;
	size_t size;
	int *dims;

	dims = MatrixDimensions("data", numdim, size);

	if (!dims)
	{
		return;
	}

	/* We override this actually */
	dims[numdim - 1] = names.size();
	size = dims[0] * dims[1] * (numdim > 2 ? dims[2] : 1);

	double *numdata = 0;
	matvar_t **data = 0;

	if (d_numericdata)
	{
		numdata = new double[size];
	}
	else
	{
		data = new matvar_t *[size];
	}

	size_t ct = 0;

	Row row(0, 0);

	if (colcomb.size() > 0)
	{
		if (d_isSystematic)
		{
			row = d_database("SELECT " + colstr + " FROM data ORDER BY `index`");
		}
		else
		{
			row = d_database("SELECT " + colstr + " FROM data ORDER BY iteration, `index`");
		}
	}
	else
	{
		Row cnt = d_database("SELECT COUNT(*) FROM data");

		d_ticker += cnt.Get<size_t>(0);
		EmitProgress();
	}

	while (row && !row.Done())
	{
		for (size_t i = 0; i < row.Length(); ++i)
		{
			String s = SafelyGetNullString(row, i);

			size_t idx = numdim == 3 ? Normalize3D(ct, dims) : Normalize2D(ct, dims);

			if (d_numericdata)
			{
				numdata[idx] = (double)s;
			}
			else
			{
				data[idx] = Serialize("", s);
			}

			++ct;
		}

		++d_ticker;
		EmitProgress();

		row.Next();
	}

	if (d_numericdata)
	{
		Write(Mat_VarCreate ("data_values",
		                     MAT_C_DOUBLE,
		                     MAT_T_DOUBLE,
		                     numdim,
		                     dims,
		                     numdata,
		                     0));

		delete[] numdata;
	}
	else
	{
		Write(Mat_VarCreate ("data_values",
		                     MAT_C_CELL,
		                     MAT_T_CELL,
		                     numdim,
		                     dims,
		                     data,
		                     0));

		delete[] data;
	}

	delete[] dims;
}

void
Exporter::ExportSolutions()
{
	ExportParameterValues();
	ExportParameterActive();
	ExportFitness();
	ExportData();
}

void
Exporter::Write(matvar_t *var)
{
	if (d_structures.empty())
	{
		Mat_VarWrite (d_matlab, var, 0);
		Mat_VarFree (var);
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

	matvar_t *st = Mat_VarCreate (name.c_str(),
	                              MAT_C_STRUCT,
	                              MAT_T_STRUCT,
	                              2,
	                              dims,
	                              data,
	                              0);

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

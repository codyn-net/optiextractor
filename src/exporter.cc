#include "exporter.hh"
#include <jessevdk/base/string.hh>
#include <iomanip>

using namespace optiextractor;
using namespace std;
using namespace jessevdk::db::sqlite;
using namespace jessevdk::base;

Exporter::Exporter(ostream &ostr, SQLite &database)
:
	d_stream(ostr),
	d_database(database),
	d_level(0)
{
	d_stream << setprecision(5);
}

void
Exporter::Export()
{
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

	Write("iteration_times", times);
}

void
Exporter::WriteNames(string const &name, jessevdk::db::sqlite::Row row, string const &prefix, string const &additional)
{
	bool first = true;

	d_stream << Indentation() << name << ": {";

	while (row && !row.Done())
	{
		String name = row.Get<string>(1);

		if (name.StartsWith(prefix))
		{
			if (!first)
			{
				d_stream << ", ";
			}

			first = false;
			d_stream << Serialize(name.substr(prefix.length()));
		}

		row.Next();
	}

	if (additional != "")
	{
		if (!first)
		{
			d_stream << ", ";
		}

		d_stream << Serialize(additional);
	}

	d_stream << "}" << endl;
}

void
Exporter::ExportSolutions()
{
	// Write the parameter names
	WriteNames("parameter_names", d_database("PRAGMA table_info(parameter_values)"), "_p_");

	Row row = d_database("SELECT * FROM parameter_values ORDER BY iteration, `index`");

	d_stream << setprecision(12);

	// Parameter values matrix
	d_stream << Indentation() << "parameter_values: []" << endl;

	Begin();
	{
		while (row && !row.Done())
		{
			vector<double> v;

			for (size_t i = 2; i < row.Length(); ++i)
			{
				v.push_back(row.Get<double>(i));
			}

			d_stream << Indentation() << Serialize(v) << endl;
			row.Next();
		}
	}
	End();

	// Write the fitness names
	WriteNames("fitness_names", d_database("PRAGMA table_info(fitness)"), "_f_", "value");

	row = d_database("SELECT * FROM fitness ORDER BY iteration, `index`");

	// Fitness matrix
	d_stream << Indentation() << "fitness_values: []" << endl;

	Begin();
	{
		while (row && !row.Done())
		{
			vector<double> v;

			for (size_t i = 3; i < row.Length(); ++i)
			{
				v.push_back(row.Get<double>(i));
			}

			v.push_back(row.Get<double>(2));

			d_stream << Indentation() << Serialize(v) << endl;
			row.Next();
		}
	}
	End();

	WriteNames("data_names", d_database("PRAGMA table_info(data)"), "_d_");

	// Matrix matrix
	d_stream << Indentation() << "data_values: {}" << endl;
	Begin();
	{
		while (row && !row.Done())
		{
			d_stream << Indentation() << "{" << row.Get<size_t>(0) << ", " << row.Get<size_t>(1);

			for (size_t i = 2; i < row.Length(); ++i)
			{
				d_stream << ", " << Serialize(row.Get<string>(i));
			}

			d_stream << "}" << endl;
			row.Next();
		}
	}
	End();

	d_stream << setprecision(5);
}

void
Exporter::Begin(string const &name)
{
	if (name != "")
	{
		d_stream << Indentation() << name << ":" << endl;
	}

	d_indentation += "\n";
	++d_level;
}

void
Exporter::End()
{
	if (d_level == 0)
	{
		return;
	}

	d_stream << endl;

	d_indentation = d_indentation.substr(1);
	--d_level;
}

string
Exporter::Serialize(string const &v) const
{
	return "'" + String(v).Replace("'", "''") + "'";
}

string
Exporter::Serialize(double v) const
{
	return SerializeStream(v);
}

string
Exporter::Serialize(size_t v) const
{
	return SerializeStream(v);
}

string
Exporter::Serialize(int v) const
{
	return SerializeStream(v);
}

string
Exporter::Indentation() const
{
	return string(d_level, '\t');
}

string
Exporter::NormalizeName(string const &name) const
{
	String ret(name);

	return ret.Replace("-", "_").Replace(" ", "_").Replace(":", "_").Replace(".", "_");
}

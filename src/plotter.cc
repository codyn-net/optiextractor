#include "plotter.hh"
#include "utils.hh"

#include <fstream>
#include <sstream>
#include <jessevdk/base/string.hh>

using namespace std;
using namespace optiextractor;
using namespace jessevdk::base;
using namespace jessevdk::db;

Plotter::Field::Field(string const &name,
                      string const &fieldname,
                      Type::Values type,
                      size_t       _index,
                      string const &color)
{
	d_data = new Data();
	AddPrivateData(d_data);

	d_data->Name = name;
	d_data->FieldName = fieldname;
	d_data->TheType = type;
	d_data->Selected = false;
	d_data->Index = _index;
	d_data->Color = color;
}

string const &
Plotter::Field::Name() const
{
	return d_data->Name;
}

string const &
Plotter::Field::Color() const
{
	return d_data->Color;
}

size_t
Plotter::Field::Index() const
{
	return d_data->Index;
}

string const &
Plotter::Field::FieldName() const
{
	return d_data->FieldName;
}

Plotter::Field::Type::Values
Plotter::Field::Type() const
{
	return d_data->TheType;
}

bool
Plotter::Field::Selected() const
{
	return d_data->Selected;
}

void
Plotter::Field::SetSelected(bool selected)
{
	if (selected != d_data->Selected)
	{
		d_data->Selected = selected;
		d_data->SelectedChanged(*this);
	}
}

signals::Signal<Plotter::Field> &
Plotter::Field::SelectedChanged()
{
	return d_data->SelectedChanged;
}

Plotter::Plotter(sqlite::SQLite &database)
:
	d_database(database),
	d_scanned(false)
{
	d_configDirectories.push_back(Glib::build_filename(Glib::get_user_config_dir(), "optiextractor"));
	d_configDirectories.push_back(Glib::build_filename(DATADIR, "optiextractor"));

	d_colors.push_back("729fcf");
	d_colors.push_back("ef2929");
	d_colors.push_back("8ae234");
	d_colors.push_back("ad8fa8");
	d_colors.push_back("fce94f");
	d_colors.push_back("fcaf3e");

	d_colors.push_back("204a87");
	d_colors.push_back("a40000");
	d_colors.push_back("4e9a06");
	d_colors.push_back("5c3566");
	d_colors.push_back("c4a000");
	d_colors.push_back("ce5c00");

	d_width = 400;
	d_height = 300;
}

void
Plotter::Scan()
{
	bool ret = true;
	d_busy(ret);

	Glib::Thread::create(sigc::mem_fun(*this, &Plotter::ScanThread), false);
}

void
Plotter::Add(Field &field)
{
	d_fields.push_back(field);

	field.SelectedChanged().Add(*this, &Plotter::FieldSelectedChanged);
}

string
Plotter::FindInConfig(string const &filename) const
{
	for (vector<string>::const_iterator iter = d_configDirectories.begin();
	     iter != d_configDirectories.end();
	     ++iter)
	{
		string full = Glib::build_filename(*iter, filename);

		if (Glib::file_test(full, Glib::FILE_TEST_IS_REGULAR))
		{
			return full;
		}
	}

	return "";
}

string
Plotter::SubstitutePlotLine(string const &line)
{
	string ret = line;

	stringstream width;
	width << (size_t)(d_width / 1.5);

	stringstream height;
	height << (size_t)(d_height / 1.5);

	ret = String(ret).Replace("%{PLOTS}", d_texPlotCommands);
	ret = String(ret).Replace("%{PREAMBLE}", d_texPreamble);
	ret = String(ret).Replace("%{WIDTH}", width.str() + "px");
	ret = String(ret).Replace("%{HEIGHT}", height.str() + "px");

	return ret;
}

void
Plotter::Resize(size_t width, size_t height)
{
	if (d_width == width && d_height == height)
	{
		return;
	}

	d_width = width;
	d_height = height;

	QueueUpdate();
}

void
Plotter::PdfLatexDone(GPid pid, int ret)
{
	Glib::spawn_close_pid(pid);

	if (pid != d_childPid)
	{
		return;
	}

	d_filename = d_currentTexFilename + ".png";

	bool rr = false;
	d_busy(rr);

	d_updated();
}

bool
Plotter::IdleUpdate()
{
	d_idleUpdate.disconnect();

	UpdateTexPlotCommands();

	// Generate tex file
	string templatefile = FindInConfig("plot.tex");
	string tempfile = GenerateTemporaryFile();
	string texfile = tempfile + ".tex";

	fstream out(texfile.c_str(), ios::out);
	fstream inp(templatefile.c_str(), ios::in);

	string line;

	while (getline(inp, line))
	{
		out << SubstitutePlotLine(line) << endl;
	}

	inp.close();
	out.close();

	string wd = Glib::path_get_dirname(tempfile);
	vector<string> argv;
	vector<string> envp;

	string path = FindInConfig("pnglatex");

	stringstream width;
	width << d_width;

	stringstream height;
	height << d_height;

	argv.push_back(path);
	argv.push_back(texfile);
	argv.push_back(width.str());
	argv.push_back(height.str());

	d_currentTexFilename = tempfile;
	GPid child_pid = 0;

	// Run pdflatex in the background
	Glib::spawn_async(wd,
	                  argv,
	                  envp,
	                  Glib::SPAWN_STDOUT_TO_DEV_NULL |
	                  Glib::SPAWN_STDERR_TO_DEV_NULL |
	                  Glib::SPAWN_DO_NOT_REAP_CHILD,
	                  sigc::slot<void>(),
	                  &child_pid);

	d_childPid = child_pid;

	Glib::signal_child_watch().connect(sigc::mem_fun(*this, &Plotter::PdfLatexDone),
	                                   child_pid);

	return false;
}

void
Plotter::UpdateTexPlotCommands()
{
	vector<Field>::iterator iter;
	vector<Field> selection = Selection();

	stringstream preamble;

	// Generate colors
	size_t i = 0;

	for (iter = d_fields.begin(); iter != d_fields.end(); ++iter)
	{
		string color = d_colors[i % d_colors.size()];
		transform(color.begin(), color.end(), color.begin(), (int (*)(int))toupper);

		preamble << "\\definecolor{color" << i++ << "}{HTML}{" << color << "}" << endl;
	}

	d_texPreamble = preamble.str();

	stringstream commands;
	i = 0;

	for (iter = selection.begin(); iter != selection.end(); ++iter)
	{
		commands << "\\addplot[color=color" << iter->Index() << "] table[x index=0, y index=" << (iter->Index() + 1) << "] {" << d_dataFilename << "};" << endl;
	}

	d_texPlotCommands = commands.str();
}

void
Plotter::QueueUpdate()
{
	if (d_scanned && !d_idleUpdate)
	{
		bool ret = true;
		d_busy(ret);

		d_idleUpdate = Glib::signal_idle().connect(sigc::mem_fun(*this, &Plotter::IdleUpdate));
	}
}

void
Plotter::FieldSelectedChanged(Field const &field)
{
	QueueUpdate();
}

string
Plotter::GenerateTemporaryFile()
{
	string tmpdir;
	char *tmpenv;

	tmpenv = getenv("TMPDIR");

	if (tmpenv == NULL)
	{
		tmpdir = "/tmp";
	}
	else
	{
		tmpdir = tmpenv;
	}

	string templ = string(tmpdir) + "/optiextractor-temp-file.XXXXXX";
	int fd = mkstemp((char *)templ.c_str());
	close(fd);

	return templ;
}

bool
Plotter::EmitFields()
{
	d_fieldsChanged();
	return false;
}

void
Plotter::ScanThread()
{
	stringstream header;
	header << "iteration";

	vector<string> names = Utils::FitnessColumns(d_database);
	stringstream fitselection;
	size_t numfit = names.size();
	size_t idx = 0;

	for (vector<string>::iterator iter = names.begin(); iter != names.end(); ++iter)
	{
		Field field(iter->substr(3), *iter, Field::Type::Fitness, idx, d_colors[idx % d_colors.size()]);
		++idx;

		if (!fitselection.str().empty())
		{
			fitselection << ", ";
		}

		fitselection << "`" << field.FieldName() << "`";
		header << "\t" << field.Name();

		Add(field);
		field.SetSelected(true);
	}

	names = Utils::ParameterColumns(d_database);
	stringstream parselection;
	size_t numpar = names.size();

	for (vector<string>::iterator iter = names.begin(); iter != names.end(); ++iter)
	{
		Field field(iter->substr(3), *iter, Field::Type::Parameter, idx, d_colors[idx % d_colors.size()]);
		++idx;

		if (!parselection.str().empty())
		{
			parselection << ", ";
		}

		parselection << "`" << field.FieldName() << "`";
		header << "\t" << field.Name();

		Add(field);
	}

	cout << "emitting" << endl;

	Glib::signal_idle().connect(sigc::mem_fun(*this, &Plotter::EmitFields));

	d_dataFilename = GenerateTemporaryFile();
	fstream fstr(d_dataFilename.c_str(), ios::out);

	fstr << header.str() << endl;

	sqlite::Row row = d_database("SELECT `iteration`, `best_id` FROM iteration ORDER BY `iteration`");

	if (row.Done())
	{
		fstr.close();
		return;
	}

	while (row)
	{
		size_t iteration = row.Get<size_t>(0);
		size_t solutionid = row.Get<size_t>(1);

		stringstream dataline;

		dataline << iteration;

		// Get all fitness components for this solution
		sqlite::Row fit = d_database()
			<< "SELECT " << fitselection.str() << " "
			<< "FROM `fitness` "
			<< "WHERE `iteration` = " << iteration << " AND "
			<< "`index` = " << solutionid
			<< sqlite::SQLite::Query::End();

		for (size_t i = 0; i < numfit; ++i)
		{
			if (!fit.Done() && fit)
			{
				dataline << "\t" << fit.Get<double>(i);
			}
			else
			{
				dataline << "\t" << 0 << endl;
			}
		}

		// Get all parameters for this solution
		sqlite::Row pars = d_database()
			<< "SELECT " << parselection.str() << " "
			<< "FROM `parameter_values` "
			<< "WHERE `iteration` = " << iteration << " AND "
			<< "`index` = " << solutionid
			<< sqlite::SQLite::Query::End();

		for (size_t i = 0; i < numpar; ++i)
		{
			if (!pars.Done() && pars)
			{
				dataline << "\t" << pars.Get<double>(i);
			}
			else
			{
				dataline << "\t" << 0 << endl;
			}
		}

		fstr << dataline.str() << endl;
		row.Next();
	}

	fstr.close();
	d_scanned = true;

	QueueUpdate();
}

vector<Plotter::Field> const &
Plotter::Fields() const
{
	return d_fields;
}

vector<Plotter::Field>
Plotter::Selection() const
{
	vector<Field>::const_iterator iter;
	vector<Field> ret;

	for (iter = d_fields.begin(); iter != d_fields.end(); ++iter)
	{
		if (iter->Selected())
		{
			ret.push_back(*iter);
		}
	}

	return ret;
}

string const &
Plotter::Filename() const
{
	return d_filename;
}

signals::Signal<> &
Plotter::Updated()
{
	return d_updated;
}

signals::Signal<> &
Plotter::FieldsChanged()
{
	return d_updated;
}

signals::Signal<bool> &
Plotter::Busy()
{
	return d_busy;
}

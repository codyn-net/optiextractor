#ifndef __OPTIEXTRACTOR_PLOTTER_H__
#define __OPTIEXTRACTOR_PLOTTER_H__

#include <jessevdk/db/db.hh>
#include <jessevdk/base/signals.hh>
#include <gtkmm.h>
#include <map>
#include <string>
#include <vector>

namespace optiextractor
{
	class Plotter
	{
		jessevdk::db::sqlite::SQLite &d_database;
		jessevdk::base::signals::Signal<> d_updated;
		std::string d_filename;
		std::string d_dataFilename;
		sigc::connection d_idleUpdate;
		std::vector<std::string> d_configDirectories;
		std::string d_texPlotCommands;
		std::vector<std::string> d_colors;
		std::string d_texPreamble;
		std::string d_currentTexFilename;
		GPid d_childPid;
		size_t d_width;
		size_t d_height;
		bool d_scanned;
		jessevdk::base::signals::Signal<bool> d_busy;
		jessevdk::base::signals::Signal<> d_fieldsChanged;

		public:
			class Field : public jessevdk::base::Object
			{
				public:
					struct Type
					{
						enum Values
						{
							Fitness,
							Parameter
						};
					};

					Field(std::string const &name,
					      std::string const &fieldname,
					      Type::Values type,
					      size_t _index,
					      std::string const &color);

					std::string const &Name() const;
					std::string const &FieldName() const;
					Type::Values Type() const;
					size_t Index() const;
					std::string const &Color() const;

					bool Selected() const;
					void SetSelected(bool selected);

					jessevdk::base::signals::Signal<Field> &SelectedChanged();
				private:
					struct Data : public jessevdk::base::Object::PrivateData
					{
						std::string Name;
						std::string FieldName;
						Type::Values TheType;
						bool Selected;
						size_t Index;
						std::string Color;

						jessevdk::base::signals::Signal<Field> SelectedChanged;
					};

					Data *d_data;
			};

			Plotter(jessevdk::db::sqlite::SQLite &database);

			std::vector<Field> const &Fields() const;
			std::vector<Field> Selection() const;

			std::string const &Filename() const;
			void Resize(size_t width, size_t height);

			void Scan();

			jessevdk::base::signals::Signal<> &Updated();
			jessevdk::base::signals::Signal<bool> &Busy();
			jessevdk::base::signals::Signal<> &FieldsChanged();
		private:
			std::vector<Field> d_fields;

			void ScanThread();
			void Add(Field &field);
			void FieldSelectedChanged(Field const &field);

			std::string GenerateTemporaryFile();
			bool IdleUpdate();

			std::string FindInConfig(std::string const &filename) const;
			std::string SubstitutePlotLine(std::string const &line);
			void UpdateTexPlotCommands();

			void PdfLatexDone(GPid pid, int ret);
			void QueueUpdate();
			bool EmitFields();
	};
}

#endif /* __OPTIEXTRACTOR_PLOTTER_H__ */

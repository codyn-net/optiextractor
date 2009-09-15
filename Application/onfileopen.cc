#include "application.ih"

void Application::onFileOpen()
{
	/* Open dialog */
	if (d_openDialog)
	{
		delete d_openDialog;
	}
	
	d_openDialog = new Gtk::FileChooserDialog(window(), 
	                                          "Open Optimization Database",
	                                          Gtk::FILE_CHOOSER_ACTION_OPEN);

	d_openDialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	d_openDialog->add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

	d_openDialog->set_local_only(true);

	d_openDialog->signal_response().connect(sigc::mem_fun(*this, &Application::openResponse));
	d_openDialog->show();
}

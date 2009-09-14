#include "application.ih"

void Application::destroyDialog(int response)
{
	if (d_dialog)
	{
		delete d_dialog;
	}
}

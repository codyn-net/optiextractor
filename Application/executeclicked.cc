#include "application.ih"

void Application::executeClicked()
{
	if (d_runner)
	{
		// Cancel it
		d_runner.cancel();
	}
	else
	{
		runSolution();
	}
}

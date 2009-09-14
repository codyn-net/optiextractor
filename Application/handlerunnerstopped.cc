#include "application.ih"

void Application::handleRunnerStopped()
{
	// Show response dialog
	cerr << "stopped: " << d_runner.error() << endl;
}

#include "application.ih"

void Application::runnerResponse(Response const &response)
{
	d_lastResponse = response;
}

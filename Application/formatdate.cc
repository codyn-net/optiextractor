#include "application.ih"

string Application::formatDate(size_t timestamp, std::string const &format) const
{
	char buffer[1024];
	time_t t = static_cast<time_t>(timestamp);

	struct tm *tt = localtime(&t);
	strftime(buffer, 1024, format.c_str(), tt);
	
	return string(buffer);
}

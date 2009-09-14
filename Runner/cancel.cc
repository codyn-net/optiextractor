#include "runner.ih"

void Runner::cancel()
{
	if (d_running)
	{
		Response response;
		
		response.mutable_failure()->set_type(Response::Failure::Disconnected);
		dispatchClose();
	}
}

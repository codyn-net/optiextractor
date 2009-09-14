#include "runner.ih"

void Runner::writeTask(optimization::messages::worker::Request::Dispatch const &dispatch)
{
	string serialized;

	Messages::create(dispatch, serialized);
	d_pipe.writeEnd().write(serialized);	
}

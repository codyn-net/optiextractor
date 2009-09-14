#include "runner.ih"

bool Runner::onDispatchData(os::FileDescriptor::DataArgs &args)
{
	vector<optimization::messages::worker::Response> messages;
	vector<optimization::messages::worker::Response>::iterator iter;
	
	Messages::extract(args, messages);
	
	for (iter = messages.begin(); iter != messages.end(); ++iter)
	{
		onResponse(*iter);
		break;
	}
	
	return false;
}

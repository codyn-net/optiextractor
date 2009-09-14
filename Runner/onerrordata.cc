#include "runner.ih"

bool Runner::onErrorData(os::FileDescriptor::DataArgs &args)
{
	d_errorMessage += args.data;
	return false;
}

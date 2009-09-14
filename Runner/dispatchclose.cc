#include "runner.ih"

void Runner::dispatchClose()
{
	d_pipe.readEnd().onData().remove(*this, &Runner::onDispatchData);
	d_pipe.readEnd().close();
	d_pipe.writeEnd().close();

	d_errorChannel.onData().remove(*this, &Runner::onErrorData);
	d_errorChannel.close();

	d_connection.disconnect();
	
	d_running = false;
	
	onState(d_running);
	d_errorMessage = "";
}

#include "runner.ih"

void Runner::onDispatchedKilled(GPid pid, int ret)
{
	Glib::spawn_close_pid(pid);
	
	dispatchClose();
}

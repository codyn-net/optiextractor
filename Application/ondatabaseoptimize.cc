#include "application.ih"

void Application::onDatabaseOptimize()
{
	/* Create indices if necessary */
	d_database.begin();
	d_database.query("CREATE INDEX IF NOT EXISTS iteration_iteration ON iteration(`iteration`)");
	d_database.query("CREATE INDEX IF NOT EXISTS solution_index ON solution(`index`)");
	d_database.query("CREATE INDEX IF NOT EXISTS solution_iteration ON solution(`iteration`)");
	
	d_database.query("CREATE INDEX IF NOT EXISTS fitness_index ON fitness(`index`)");
	d_database.query("CREATE INDEX IF NOT EXISTS fitness_iteration ON fitness(`iteration`)");

	d_database.commit();
}

#include <cstdlib>
#include <ctime>
#include <iostream>

#include <sys/timeb.h>

int main()
{
	timeb t_start, t_end;
	int msec_diff;

	ftime(&t_start);
	system("ping -c 1 yahoo.com");
	ftime(&t_end);

	msec_diff = (1000 * t_end.time + t_end.millitm) - (1000 * t_start.time + t_start.millitm);

	std::cout << msec_diff << "ms\n";

	return 0;
}

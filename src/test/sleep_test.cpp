
#ifdef _WIN32
# include <windows.h>
#else
# include <sys/time.h>
# include <time.h>
# include <sched.h>
# include <asm/param.h>
# include <unistd.h>
#endif

#include <iostream>

// The purpose of this test is to experiment with the precision of the sleep
// functions on Linux, OSX, and Windows.
// On Linux, nanosleep() and usleep() always wait for longer than requested,
// usually by more than 10 ms.

// On OSX, nanosleep() is very precise, consistantly overshooting the mark by 
// only about 30 usec.

// On Windows XP, Sleep() usually waits for less than that requested, and
// occasionally more than requested.  The Sleep time granularity on Windows appears
// to be roughly 16 ms, with no fractions.  A request of 0 returns immediately.
// A request between 1-15 causes a short delay of 300-900 usec.  A second call
// immediately after the first introduces a full 16 ms wait.

int
main(int argc, char** argv) 
{
	int ms = 5;
	if (argc == 2)
		ms = strtol( argv[1], 0, 0);

#ifdef _WIN32
	DWORD sleep_time = ms;
	LARGE_INTEGER _tics_per_second;
	QueryPerformanceFrequency( &_tics_per_second);
	long long tics_per_second = _tics_per_second.QuadPart;
	long long begin = 0;
	long long end = 0;
	QueryPerformanceCounter( (LARGE_INTEGER*)&begin);
	QueryPerformanceCounter( (LARGE_INTEGER*)&end);
	long long timer_delay = (end - begin) * 1000000 / tics_per_second;
	std::cout << "Clock latency: " << timer_delay << " usec.\n";
	std::cout << "Requested delay: " << ms << " msec.\n";
	
	QueryPerformanceCounter( (LARGE_INTEGER*)&begin);
	Sleep( sleep_time);
	QueryPerformanceCounter( (LARGE_INTEGER*)&end);
	
	std::cout << "Sleep delay: " << (end - begin) * 1000000 / tics_per_second << " usec.\n";

#else
	sched_param pri;
	pri.sched_priority = 1;
	sched_setscheduler( 0, SCHED_RR, &pri);
	if (errno == EPERM) {
		std::cerr << "Cound not set scheduling policy: Inadequate permission.\n";
	}
	else if (errno == EINVAL) {
		std::cerr << "Cound not set scheduling policy: Invalid argument.\n";
	}
	else if (errno == ESRCH) {
		std::cerr << "Could not set scheduling policy: pid not found.\n";
	}
	
	timespec sleep_time = { 0, ms * 1000000};
	timespec remain = { 0, 0};
	
	timeval begin;
	timeval end;
	timerclear( &begin);
	timerclear( &end);
	
	gettimeofday( &begin, 0);
	gettimeofday( &end, 0);
	long timer_delay = end.tv_usec - begin.tv_usec;	
	timerclear( &begin);
	timerclear( &end);
	
	std::cout << "Clock latency: " << timer_delay << " usec.\n";
	std::cout << "Requested delay: " << sleep_time.tv_nsec / 1000 << " usec.\n";
	
	gettimeofday( &begin, 0);
	nanosleep( &sleep_time, &remain);
	gettimeofday( &end, 0);
	
	std::cout << "nanosleep delay: " << end.tv_usec - begin.tv_usec << " usec with "
		<< remain.tv_nsec / 1000 << " usec remaining.\n";
	std::cout << "Claimed resolution: " << 1.0/HZ * 1e6 << " usec.\n";
	timerclear( &begin);
	timerclear( &end);

	gettimeofday( &begin, 0);
	usleep( ms * 1000);
	gettimeofday( &end, 0);
	std::cout << "usleep delay: " << end.tv_usec - begin.tv_usec << " usec.\n";
#endif

	return 0;
}

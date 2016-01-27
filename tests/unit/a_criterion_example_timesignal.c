#include <signal.h>
#include <criterion/criterion.h>

#ifdef _WIN32
# include <Windows.h>
# define sleep(x) Sleep(x * 1000)
#else
# include <unistd.h>
#endif

// Cross platform segfault simulator ™
// a.k.a. "I can't believe I have to write this for a sample"

#ifdef _WIN32
# include <windows.h>
#endif

void crash(void) {
#ifdef _WIN32
    // This translates to a SIGSEGV
    RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0, NULL);
#else
    raise(SIGSEGV);
#endif
}

Test(simple, timeout, .timeout = 1.0, .signal = SIGALRM) {
    sleep(10);
}

Test(simple, caught, .signal = SIGSEGV) {
    crash();
}

Test(simple, wrong_signal, .signal = SIGINT) {
    crash();
}

Test(simple, uncaught) {
    crash();
}

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/unistd.h>

// Dummy syscall implementations to avoid linker warnings.

extern "C" int _write(int file, char *ptr, int len) {
  (void)file;
  (void)ptr;
  (void)len;
  return -1;
}

extern "C" int _read(int file, char *ptr, int len) {
  (void)file;
  (void)ptr;
  (void)len;
  return -1;
}

extern "C" int _close(int file) {
  (void)file;
  return -1;
}

extern "C" int _fstat(int file, struct stat *st) {
  (void)file;
  (void)st;
  return -1;
}

extern "C" int _isatty(int file) {
  (void)file;
  return -1;
}

extern "C" int _lseek(int file, int ptr, int dir) {
  (void)file;
  (void)ptr;
  (void)dir;
  return -1;
}

extern "C" int _getpid(void) { return -1; }

extern "C" int _kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  return -1;
}

extern "C" void *_sbrk(ptrdiff_t incr) {
  (void)incr;
  return (void *)-1;
}

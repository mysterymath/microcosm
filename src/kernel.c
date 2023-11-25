#include <zephyr/sys/printk.h>

extern char hello[];

void main(void) {
  void (*_start)(void (*)(const char *fmt, ...)) = (void (*)(
      void (*)(const char *fmt, ...)))(hello + 74 + 1);
  _start(printk);
}

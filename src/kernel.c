#include <zephyr/sys/printk.h>

extern char hello[];

void main(void) {
  // bit[0] of the address must be set to indicate thumb
  void (*_start)(void (*)(const char *fmt, ...)) = (void (*)(
      void (*)(const char *fmt, ...)))((unsigned)hello + 0x8000 | 1);
  _start(printk);
}

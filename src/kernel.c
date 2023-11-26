#include <zephyr/sys/printk.h>
#include <elf.h>

extern char hello[];

void exec(const Elf32_Ehdr* elf) {
  void (*_start)(void (*)(const char *fmt, ...)) = (void (*)(
      void (*)(const char *fmt, ...)))(hello + 74 + elf->e_entry);
  _start(printk);
}

void main(void) {
  exec((const Elf32_Ehdr*)hello);
}

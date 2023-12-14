void printk(const char *fmt, ...);

__attribute__((constructor)) void cons() {
  printk("Hello constructor!\n");
}

void _start(void) {
  printk("Hello, world!\n");
}

__attribute__((destructor)) void dest() {
  printk("Hello destructor!\n");
}

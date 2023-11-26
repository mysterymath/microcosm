#include <elf.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

extern char hello[];

// Return minimum and maximum LOADed virtual addresses.
static void get_extents(const Elf32_Ehdr *elf, uint32_t *begin, uint32_t *end) {
  *begin = UINT32_MAX;
  *end = 0;
  const Elf32_Phdr *phdr = (const Elf32_Phdr *)((char *)elf + elf->e_phoff);
  for (uint16_t i = 0; i < elf->e_phnum;
       i++, phdr = (const Elf32_Phdr *)((char *)phdr + elf->e_phentsize)) {
    if (phdr->p_type != PT_LOAD)
      continue;
    if (phdr->p_vaddr < *begin)
      *begin = phdr->p_vaddr;
    size_t cur_end = phdr->p_vaddr + phdr->p_memsz;
    if (cur_end > *end)
      *end = cur_end;
  }
}

// Load an ELF file with the given vaddr begin into an image.
static void load(const Elf32_Ehdr *elf, void *image, uint32_t begin) {
  const Elf32_Phdr *phdr = (const Elf32_Phdr *)((char *)elf + elf->e_phoff);
  for (uint16_t i = 0; i < elf->e_phnum;
       i++, phdr = (const Elf32_Phdr *)((char *)phdr + elf->e_phentsize)) {
    char *dst = (char *)image + (phdr->p_vaddr - begin);
    char *src = (char *)elf + phdr->p_offset;
    memcpy(dst, src, phdr->p_filesz);
    memset(dst + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
  }
}

static void exec(const Elf32_Ehdr *elf) {
  uint32_t begin, end;
  get_extents(elf, &begin, &end);
  void *image = k_malloc(end - begin);
  if (!image) {
    printk("could not allocate image");
    k_panic();
  }
  load(elf, image, begin);
  void (*_start)(void (*)(const char *fmt, ...)) =
      (void (*)(void (*)(const char *fmt, ...)))((char *)image + elf->e_entry);
  _start(printk);
}

void main(void) { exec((const Elf32_Ehdr *)hello); }

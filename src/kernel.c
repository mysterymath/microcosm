#include <elf.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

_Alignas(4) static char hello_bin[] = {
#include <hello.inc>
};

_Alignas(16) char hello_mem[2048];

const Elf32_Phdr *phdr_begin(const Elf32_Ehdr *elf) {
  return (const Elf32_Phdr *)((char *)elf + elf->e_phoff);
}

const Elf32_Phdr *phdr_next(const Elf32_Phdr *phdr, const Elf32_Ehdr *elf) {
  return (const Elf32_Phdr *)((char *)phdr + elf->e_phentsize);
}

static uint32_t elf_base(const Elf32_Ehdr *elf) {
  const Elf32_Phdr *phdr = phdr_begin(elf);
  for (uint16_t i = 0; i < elf->e_phnum; i++, phdr = phdr_next(phdr, elf))
    if (phdr->p_type == PT_LOAD)
      return phdr->p_vaddr;

  printk("missing LOAD program header");
  k_panic();
  __builtin_unreachable();
}

// Load an ELF file with the given vaddr begin into an image.
static void load(const Elf32_Ehdr *elf, void *image, size_t max_size) {
  const Elf32_Phdr *phdr = phdr_begin(elf);
  for (uint16_t i = 0; i < elf->e_phnum; i++, phdr = phdr_next(phdr, elf)) {
    if (phdr->p_type != PT_LOAD)
      continue;

    if (phdr->p_vaddr > max_size || phdr->p_memsz > max_size - phdr->p_vaddr) {
      printk("image too big");
      k_panic();
    }

    char *dst = (char *)image + phdr->p_vaddr;
    char *src = (char *)elf + phdr->p_offset;
    memcpy(dst, src, phdr->p_filesz);
    memset(dst + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
  }
}

const Elf32_Dyn *dyn_begin(const Elf32_Ehdr *elf) {
  const Elf32_Phdr *phdr = phdr_begin(elf);
  for (uint16_t i = 0; i < elf->e_phnum; i++, phdr = phdr_next(phdr, elf)) {
    if (phdr->p_type == PT_DYNAMIC)
      return (const Elf32_Dyn *)((const char *)elf + phdr->p_offset);
  }
  return NULL;
}

typedef struct {
  Elf32_Ehdr *elf;

  uint32_t jmprel;
  uint32_t pltrelsz;
  uint32_t strtab;
  uint32_t syment;
  uint32_t symtab;
} Dynamic;

Dynamic dyn_parse(Elf32_Ehdr *elf) {
  Dynamic ret;
  ret.elf = elf;
  for (const Elf32_Dyn *dyn = dyn_begin(elf); dyn->d_tag != DT_NULL; ++dyn) {
    if (dyn->d_tag == DT_JMPREL)
      ret.jmprel = dyn->d_un.d_val;
    else if (dyn->d_tag == DT_PLTRELSZ)
      ret.pltrelsz = dyn->d_un.d_val;
    else if (dyn->d_tag == DT_STRTAB)
      ret.strtab = dyn->d_un.d_ptr;
    else if (dyn->d_tag == DT_SYMENT)
      ret.syment = dyn->d_un.d_val;
    else if (dyn->d_tag == DT_SYMTAB)
      ret.symtab = dyn->d_un.d_ptr;
  }
  return ret;
}

const Elf32_Rel *dyn_jmprel_begin(const Dynamic *dyn) {
  return (const Elf32_Rel *)((const char *)dyn->elf + dyn->jmprel);
}
const Elf32_Rel *dyn_jmprel_end(const Dynamic *dyn) {
  return (const Elf32_Rel *)((const char *)dyn->elf + dyn->jmprel +
                             dyn->pltrelsz);
}

const Elf32_Sym *dyn_sym(const Dynamic *dyn, uint32_t idx) {
  return (const Elf32_Sym *)((const char *)dyn->elf + dyn->symtab +
                             idx * dyn->syment);
}

const char *dyn_str(const Dynamic *dyn, uint32_t idx) {
  return (const char *)dyn->elf + dyn->strtab + idx;
}

static void link(Elf32_Ehdr *elf) {
  const uint32_t elf_offset = (uint32_t)elf - elf_base(elf);
  Dynamic dyn = dyn_parse(elf);
  for (const Elf32_Rel *rel = dyn_jmprel_begin(&dyn),
                       *end = dyn_jmprel_end(&dyn);
       rel != end; ++rel) {

    const Elf32_Sym *sym = dyn_sym(&dyn, ELF32_R_SYM(rel->r_info));
    const char *name = sym->st_name ? dyn_str(&dyn, sym->st_name) : NULL;
    uint32_t value = 0;
    if (name) {
      if (!strcmp(name, "printk")) {
        value = (uint32_t)printk;
      } else if (ELF32_ST_BIND(sym->st_info) != STB_WEAK) {
        printk("unknown symbol: %s\n", name);
        k_panic();
      }
    }

    unsigned char type = ELF32_R_TYPE(rel->r_info);
    switch (type) {
      default:
        printk("unhandled relocation: %d\n", type);
        k_panic();
        __builtin_unreachable();
      case R_ARM_JUMP_SLOT:
        *(uint32_t *)((const char *)elf + rel->r_offset) = value;
        break;
      case R_ARM_RELATIVE:
        *(uint32_t *)((const char *)elf + rel->r_offset) += elf_offset;
        break;
    }
  }
}

static void exec(const Elf32_Ehdr *elf) {
  load(elf, hello_mem, sizeof hello_mem);
  Elf32_Ehdr *hello = (Elf32_Ehdr *)hello_mem;
  link(hello);
  void (*_start)(void) = (void (*)(void))((const char *)hello + hello->e_entry);
  _start();
}

int main(void) {
  exec((const Elf32_Ehdr *)hello_bin);
  return 0;
}

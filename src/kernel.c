#include <elf.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

_Alignas(4) static char hello_bin[] = {
#include <hello.inc>
};

_Alignas(16) char hello_mem[2048];

static void *ptr_offset(void *elf, uint32_t offset) {
  return (char *)elf + offset;
}

const Elf32_Phdr *phdr_begin(const Elf32_Ehdr *elf) {
  return ptr_offset((void *)elf, elf->e_phoff);
}

const Elf32_Phdr *phdr_next(const Elf32_Phdr *phdr, const Elf32_Ehdr *elf) {
  return ptr_offset((void *)phdr, elf->e_phentsize);
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

    char *dst = ptr_offset(image, phdr->p_vaddr);
    char *src = ptr_offset((void *)elf, phdr->p_offset);
    memcpy(dst, src, phdr->p_filesz);
    memset(dst + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
  }
}

const Elf32_Dyn *dyn_begin(const Elf32_Ehdr *elf) {
  const Elf32_Phdr *phdr = phdr_begin(elf);
  for (uint16_t i = 0; i < elf->e_phnum; i++, phdr = phdr_next(phdr, elf))
    if (phdr->p_type == PT_DYNAMIC)
      return ptr_offset((void *)elf, phdr->p_offset);
  return NULL;
}

typedef struct {
  Elf32_Ehdr *elf;

  uint32_t rel;
  uint32_t relsz;
  uint32_t relent;
  uint32_t jmprel;
  uint32_t pltrelsz;
  uint32_t strtab;
  uint32_t syment;
  uint32_t symtab;
  uint32_t init_array;
  uint32_t init_arraysz;
  uint32_t fini_array;
  uint32_t fini_arraysz;
} Dynamic;

Dynamic dyn_parse(Elf32_Ehdr *elf) {
  Dynamic ret = {0};
  ret.elf = elf;
  for (const Elf32_Dyn *dyn = dyn_begin(elf); dyn->d_tag != DT_NULL; ++dyn) {
    switch (dyn->d_tag) {
    case DT_REL:
      ret.rel = dyn->d_un.d_ptr;
      break;
    case DT_RELSZ:
      ret.relsz = dyn->d_un.d_val;
      break;
    case DT_RELENT:
      ret.relent = dyn->d_un.d_val;
      break;
    case DT_JMPREL:
      ret.jmprel = dyn->d_un.d_val;
      break;
    case DT_PLTRELSZ:
      ret.pltrelsz = dyn->d_un.d_val;
      break;
    case DT_STRTAB:
      ret.strtab = dyn->d_un.d_ptr;
      break;
    case DT_SYMENT:
      ret.syment = dyn->d_un.d_val;
      break;
    case DT_SYMTAB:
      ret.symtab = dyn->d_un.d_ptr;
      break;
    case DT_INIT_ARRAY:
      ret.init_array = dyn->d_un.d_ptr;
      break;
    case DT_INIT_ARRAYSZ:
      ret.init_arraysz = dyn->d_un.d_val;
      break;
    case DT_FINI_ARRAY:
      ret.fini_array = dyn->d_un.d_ptr;
      break;
    case DT_FINI_ARRAYSZ:
      ret.fini_arraysz = dyn->d_un.d_val;
      break;
    }
  }
  return ret;
}

const Elf32_Rel *dyn_rel(const Dynamic *dyn, uint32_t offset) {
  return ptr_offset(dyn->elf, dyn->rel + offset);
}

const Elf32_Rel *dyn_jmprel_begin(const Dynamic *dyn) {
  return ptr_offset(dyn->elf, dyn->jmprel);
}
const Elf32_Rel *dyn_jmprel_end(const Dynamic *dyn) {
  return ptr_offset(dyn->elf, dyn->jmprel + dyn->pltrelsz);
}

const Elf32_Sym *dyn_sym(const Dynamic *dyn, uint32_t idx) {
  return ptr_offset(dyn->elf, dyn->symtab + idx * dyn->syment);
}

const char *dyn_str(const Dynamic *dyn, uint32_t idx) {
  return ptr_offset(dyn->elf, dyn->strtab + idx);
}

typedef void (*init_fini_fn)();

static void relocate(const Elf32_Rel *rel, const Dynamic *dyn,
                     uint32_t elf_offset) {
  const Elf32_Sym *sym = dyn_sym(dyn, ELF32_R_SYM(rel->r_info));
  const char *name = sym->st_name ? dyn_str(dyn, sym->st_name) : NULL;
  uint32_t value = 0;
  if (name) {
    if (!strcmp(name, "printk")) {
      value = (uint32_t)printk;
    } else if (ELF32_ST_BIND(sym->st_info) != STB_WEAK) {
      printk("unknown symbol: %s\n", name);
      k_panic();
    }
  }

  uint32_t *loc = (uint32_t *)((char *)dyn->elf + rel->r_offset);

  unsigned char type = ELF32_R_TYPE(rel->r_info);
  switch (type) {
  default:
    printk("unhandled relocation: %d\n", type);
    k_panic();
    __builtin_unreachable();
  case R_ARM_GLOB_DAT:
    *loc += value;
    break;
  case R_ARM_JUMP_SLOT:
    *loc = value;
    break;
  case R_ARM_RELATIVE:
    *loc += elf_offset;
    break;
  }
}

static void link(Dynamic *dyn) {
  const uint32_t elf_offset = (uint32_t)dyn->elf - elf_base(dyn->elf);

  for (uint32_t rel_offset = 0; rel_offset < dyn->relsz;
       rel_offset += dyn->relent)
    relocate(dyn_rel(dyn, rel_offset), dyn, elf_offset);

  for (const Elf32_Rel *rel = dyn_jmprel_begin(dyn), *end = dyn_jmprel_end(dyn);
       rel != end; ++rel)
    relocate(rel, dyn, elf_offset);
}

static void exec(const Elf32_Ehdr *elf) {
  load(elf, hello_mem, sizeof hello_mem);
  Elf32_Ehdr *hello = (Elf32_Ehdr *)hello_mem;

  Dynamic dyn = dyn_parse(hello);
  link(&dyn);

  for (init_fini_fn *
           fn = ptr_offset(dyn.elf, dyn.init_array),
          *end = ptr_offset(dyn.elf, dyn.init_array + dyn.init_arraysz);
       fn != end; ++fn)
    (*fn)();

  void (*_start)(void) = (void (*)(void))((const char *)hello + hello->e_entry);
  _start();

  for (init_fini_fn *
           fn = ptr_offset(dyn.elf, dyn.fini_array),
          *end = ptr_offset(dyn.elf, dyn.fini_array + dyn.fini_arraysz);
       fn != end; ++fn)
    (*fn)();
}

int main(void) {
  exec((const Elf32_Ehdr *)hello_bin);
  return 0;
}

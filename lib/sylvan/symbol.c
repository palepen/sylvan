#include <dwarf.h>
#include <libdwarf.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <sylvan/inferior.h>
#include "error.h"
#include "sylvan.h"
#include "symbol.h"


static sylvan_code_t
sylvan_sym_table_init(struct sylvan_sym_table *sym_table) {
    assert(sym_table);

    sym_table->count = 0;
    sym_table->capacity = 256;
    sym_table->symbols = malloc(sym_table->capacity * sizeof(struct symbol));
    if (!sym_table->symbols)
        return sylvan_set_code(SYLVANC_OUT_OF_MEMORY);

    return SYLVANC_OK;
}

SYLVAN_INTERNAL sylvan_code_t
sylvan_sym_init(struct sylvan_inferior *inf) {
    assert(inf);

    sylvan_code_t code;
    if ((code = sylvan_sym_table_init(&inf->elf_table)))
        return code;

    if ((code = sylvan_sym_table_init(&inf->dwarf_table)))
        return code;

    return SYLVANC_OK;
}

static sylvan_code_t
sylvan_sym_table_destroy(struct sylvan_sym_table *sym_table) {    
    if (!sym_table)
        return SYLVANC_OK;

    int count = sym_table->count;
    for (int i = 0; i < count; i++)
        free(sym_table->symbols[i].name);
    free(sym_table->symbols);

    sym_table->capacity = 0;
    sym_table->count = 0;
    sym_table->symbols = NULL;

    return SYLVANC_OK;
}

SYLVAN_INTERNAL sylvan_code_t
sylvan_sym_destroy(struct sylvan_inferior *inf) {
    assert(inf);

    sylvan_code_t code;
    if ((code = sylvan_sym_table_destroy(&inf->elf_table)))
        return code;

    if ((code = sylvan_sym_table_destroy(&inf->dwarf_table)))
        return code;

    return SYLVANC_OK;
}

static sylvan_code_t
sylvan_sym_add(struct sylvan_sym_table *sym_table, const char *name, uintptr_t addr) {
    assert(sym_table);

    if (sym_table->count == sym_table->capacity) {
        sym_table->capacity <<= 1;
        struct symbol *symbols = realloc(sym_table->symbols, sym_table->capacity * sizeof(struct symbol));
        if (!symbols)
            return sylvan_set_code(SYLVANC_OUT_OF_MEMORY);
        sym_table->symbols = symbols;
    }

    sym_table->symbols[sym_table->count].name = strdup(name);
    sym_table->symbols[sym_table->count].addr = addr;
    sym_table->count++;

    return SYLVANC_OK;
}

static int
symcmp(const void *l, const void *r) {
    const struct symbol *syml = l;
    const struct symbol *symr = r;
    return strcmp(syml->name, symr->name);
}

static struct symbol *
sym_lookup(struct sylvan_sym_table *sym_table, const char *name) {
    struct symbol key;
    key.name = (char *)name;
    return bsearch(&key, sym_table->symbols, sym_table->count, sizeof(struct symbol), symcmp);
}

static sylvan_code_t
sylvan_sym_load_dwarf(const char *path, struct sylvan_sym_table *sym_table) {
    Dwarf_Debug dbg = 0;
    Dwarf_Error err;
    Dwarf_Unsigned next_cu_header = 0;

    if (dwarf_init_path(path, NULL, 0, DW_GROUPNUMBER_ANY, NULL, NULL, &dbg, &err) != DW_DLV_OK)
        return SYLVANC_DWARF_NOT_FOUND;

    for (;;) {
        Dwarf_Die cu_die = 0;
        Dwarf_Unsigned cu_header_length = 0;
        Dwarf_Half version_stamp = 0;
        Dwarf_Unsigned abbrev_offset = 0;
        Dwarf_Half address_size = 0;
        Dwarf_Half offset_size = 0;
        Dwarf_Half extension_size = 0;
        Dwarf_Sig8 signature;
        Dwarf_Unsigned typeoffset = 0;
        Dwarf_Half header_cu_type = 0;
        Dwarf_Bool is_info = true;

        int res = dwarf_next_cu_header_e(
            dbg, is_info, &cu_die, &cu_header_length, &version_stamp,
            &abbrev_offset, &address_size, &offset_size, &extension_size,
            &signature, &typeoffset, &next_cu_header, &header_cu_type, &err);

        if (res == DW_DLV_NO_ENTRY)
            break;

        if (res == DW_DLV_ERROR) {
            dwarf_finish(dbg);
            return SYLVANC_OK;
        }

        Dwarf_Die child_die;
        if (dwarf_child(cu_die, &child_die, &err) != DW_DLV_OK)
            continue;

        for (;;) {
            Dwarf_Half tag;
            if (dwarf_tag(child_die, &tag, &err) != DW_DLV_OK)
                break;

            if (tag == DW_TAG_subprogram) {
                char *name = NULL;
                if (dwarf_diename(child_die, &name, &err) == DW_DLV_OK && name) {
                    Dwarf_Addr lowpc;
                    if (dwarf_lowpc(child_die, &lowpc, &err) == DW_DLV_OK)
                        sylvan_sym_add(sym_table, name, (unsigned long)lowpc);
                }
            }

            Dwarf_Die next_die;
            int sres = dwarf_siblingof_b(dbg, child_die, true, &next_die, &err);
            if (sres == DW_DLV_NO_ENTRY)
                break;
            if (sres == DW_DLV_ERROR)
                break;
            child_die = next_die;
        }
    }

    dwarf_finish(dbg);
    return SYLVANC_OK;

}

static sylvan_code_t
sylvan_sym_load_elf(const char *path, struct sylvan_sym_table *sym_table) {
    if (elf_version(EV_CURRENT) == EV_NONE)
        return sylvan_set_code(SYLVANC_ELF_FAILED);

    int fd;
    if ((fd = open(path, O_RDONLY)) < 0)
        return sylvan_set_errno_msg(SYLVANC_ELF_FAILED, "open");

    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf)
        goto fail;

    size_t shstrndx;
    if (elf_getshdrstrndx(elf, &shstrndx) != 0)
        goto fail;

    Elf_Scn *scn = NULL;
    while ((scn = elf_nextscn(elf, scn))) {
        GElf_Shdr shdr;
        if (!gelf_getshdr(scn, &shdr))
            continue;

        if (shdr.sh_type != SHT_SYMTAB && shdr.sh_type != SHT_DYNSYM)
            continue;

        Elf_Data *data = elf_getdata(scn, NULL);
        if (!data)
            continue;

        int count = shdr.sh_size / shdr.sh_entsize;
        for (int i = 0; i < count; ++i) {
            GElf_Sym sym;
            if (!gelf_getsym(data, i, &sym))
                continue;

            GElf_Shdr target_shdr;
            Elf_Scn *target_scn = elf_getscn(elf, sym.st_shndx);
            if (!target_scn || !gelf_getshdr(target_scn, &target_shdr))
                continue;

            if (!(target_shdr.sh_flags & SHF_EXECINSTR))
                continue;

            const char *name = elf_strptr(elf, shdr.sh_link, sym.st_name);
            if (name && *name)
                sylvan_sym_add(sym_table, name, (uintptr_t)sym.st_value);
        }
    }

    elf_end(elf);
    close(fd);
    return SYLVANC_OK;

fail:
    close(fd);
    return sylvan_set_code(SYLVANC_ELF_FAILED);
}


SYLVAN_INTERNAL sylvan_code_t
sylvan_get_label_addr(struct sylvan_inferior *inf, const char *name, uintptr_t *addr) {
    assert(name && addr);
    struct symbol *sym;
    if ((sym = sym_lookup(&inf->dwarf_table, name)))
        *addr = sym->addr;
    else
    if ((sym = sym_lookup(&inf->elf_table, name)))
        *addr = sym->addr;
    else
        return sylvan_set_message(SYLVANC_SYMBOL_NOT_FOUND, "%.256s not found", name);
    return SYLVANC_OK;
}

SYLVAN_INTERNAL sylvan_code_t
sylvan_sym_load_tables(struct sylvan_inferior *inf) {
    assert(inf);

    sylvan_sym_destroy(inf);
    sylvan_sym_init(inf);

    sylvan_code_t code;
    if ((code = sylvan_sym_load_dwarf(inf->realpath, &inf->dwarf_table)) && code != SYLVANC_DWARF_NOT_FOUND)
        return code;

    if ((code = sylvan_sym_load_elf(inf->realpath, &inf->elf_table)))
        return code;

    qsort(inf->dwarf_table.symbols, inf->dwarf_table.count, sizeof(struct symbol), symcmp);
    qsort(inf->elf_table.symbols, inf->elf_table.count, sizeof(struct symbol), symcmp);

    return SYLVANC_OK;
}

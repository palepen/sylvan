#include <stdio.h>
#include <fcntl.h>
#include <elf.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "auxv.h"

#define DEFINE_AUXV_TYPE(type, name, desc) {type, name, desc}
static const struct auxv_name auxv_names[] = {
#include "details/auxv_types.h"
    {-1, NULL, NULL} // Sentinel
};
#undef DEFINE_AUXV_TYPE

/**
 * @brief Reads raw auxiliary vector data from /proc/<pid>/auxv
 *
 * This function constructs the path to the auxv file for the given inferior process,
 * opens it in read-only mode, and reads its contents into a dynamically allocated buffer.
 * The buffer size is fixed at 4096 bytes, which is typically sufficient for auxv data.
 * The caller is responsible for freeing the returned buffer.
 *
 * @param[in] inf Pointer to the inferior process structure containing the PID
 * @param[out] len Pointer to a size_t where the number of bytes read will be stored
 * @return Pointer to the allocated buffer containing raw auxv data, or NULL on failure
 * @note On failure, an error message is printed to stderr using fprintf()
 */
unsigned char *target_read_auxv(struct sylvan_inferior *inf, size_t *len)
{

    // Construct the path to /proc/<pid>/auxv
    char path[32];
    if (snprintf(path, sizeof(path), "/proc/%d/auxv", inf->pid) >= (int)sizeof(path))
    {
        fprintf(stderr, "Pid Error: PID too large for path buffer\n");
        return NULL;
    }

    // Open the auxv file in read-only mode
    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "Read Error: Failed to open %s\n", path);
        return NULL;
    }

    // Initial buffer size (4096 bytes should suffice for typical auxv data)
    const size_t initial_size = 4096;
    unsigned char *buffer = malloc(initial_size);
    if (!buffer)
    {
        close(fd);
        fprintf(stderr, "Memory Error: Failed to allocate memory for auxv buffer\n");
        return NULL;
    }

    // Read the entire file into the buffer
    ssize_t bytes_read = read(fd, buffer, initial_size);
    if (bytes_read < 0)
    {
        free(buffer);
        close(fd);
        fprintf(stderr, "Read Error: Failed to read %s\n", path);
        return NULL;
    }

    if ((size_t)bytes_read == initial_size)
    {
        free(buffer);
        close(fd);
        fprintf(stderr, "Error: Auxv data exceeds buffer size (%zu bytes)\n", initial_size);
        return NULL;
    }

    close(fd);
    *len = (size_t)bytes_read;
    return buffer;
}

/**
 * @brief Parses raw auxiliary vector data into an array of entries
 *
 * This function takes raw auxv data and converts it into an array of auxv_entry structures,
 * determining the entry size based on whether the process is 64-bit or 32-bit.
 * It iterates through the data until an AT_NULL entry is found or the data length is exceeded,
 * and ensures the resulting array is null-terminated. The caller must free the returned array.
 *
 * @param[in] data Pointer to the raw auxv data buffer
 * @param[in] len Length of the raw auxv data in bytes
 * @param[in] is_64bit Flag indicating if the process is 64-bit (1) or 32-bit (0)
 * @return Pointer to an array of auxv_entry structures, or NULL on failure
 * @note The function assumes the data is properly aligned for the architecture
 */
struct auxv_entry *parse_auxv(const unsigned char *data, size_t len, int is_64bit)
{
    if (!data || len == 0)
    {
        return NULL;
    }

    // Determine entry size based on architecture bitness
    size_t entry_size = is_64bit ? sizeof(Elf64_auxv_t) : sizeof(Elf32_auxv_t);
    if (len < entry_size)
    {
        return NULL; // Not enough data for even one entry
    }

    // Calculate maximum number of entries, including space for AT_NULL terminator
    size_t max_entries = len / entry_size + 1;
    struct auxv_entry *entries = calloc(max_entries, sizeof(struct auxv_entry));
    if (!entries)
    {
        return NULL; // Memory allocation failed
    }

    size_t i = 0;
    if (is_64bit)
    {
        // Parse as 64-bit entries
        const Elf64_auxv_t *auxv = (const Elf64_auxv_t *)data;
        while (i * entry_size < len && auxv[i].a_type != AT_NULL)
        {
            entries[i].type = auxv[i].a_type;
            entries[i].value = auxv[i].a_un.a_val;
            i++;
        }
    }
    else
    {
        // Parse as 32-bit entries
        const Elf32_auxv_t *auxv = (const Elf32_auxv_t *)data;
        while (i * entry_size < len && auxv[i].a_type != AT_NULL)
        {
            entries[i].type = auxv[i].a_type;
            entries[i].value = auxv[i].a_un.a_val;
            i++;
        }
    }

    // Add the AT_NULL terminator
    entries[i].type = AT_NULL;
    entries[i].value = 0;

    return entries;
}

/**
 * @brief Prints a single auxiliary vector entry in a human-readable format
 *
 * This function takes an auxv entry and prints its type, value, and a descriptive name.
 * It uses a lookup table to map known auxv types to their names, defaulting to "Unknown"
 * for unrecognized types. The output format is: type (decimal), value (hex), name.
 *
 * @param[in] inf Pointer to the inferior process structure (currently unused)
 * @param[in] entry Pointer to the auxv_entry structure to print
 * @note The value is printed as a 16-digit hexadecimal number with leading zeros
 */
void print_auxv_entry(struct auxv_entry *entry)
{

    // Find the type name or default to "Unknown"
    const char *name = "Unknown";
    const char *desc = "Unkown Auxillary Vector Entry";
    for (int j = 0; auxv_names[j].type != -1; j++)
    {
        if (auxv_names[j].type == entry->type)
        {
            name = auxv_names[j].name;
            desc = auxv_names[j].desc;
            break;
        }
    }
    // Adjust value display: decimal for sizes/IDs, hex for addresses
    if (entry->type == AT_PAGESZ || entry->type == AT_PHENT ||
        entry->type == AT_PHNUM || entry->type == AT_UID ||
        entry->type == AT_EUID || entry->type == AT_GID ||
        entry->type == AT_EGID || entry->type == AT_CLKTCK ||
        entry->type == AT_MINSIGSTKSZ || entry->type == AT_RSEQ_FEATURE_SIZE ||
        entry->type == AT_RSEQ_ALIGN)
    {
        printf("%-4ld %-21lu %-20s %s\n", entry->type, entry->value, name, desc);
    }
    else
    {
        printf("%-4ld 0x%-19lx %-20s %s\n", entry->type, entry->value, name, desc);
    }
}
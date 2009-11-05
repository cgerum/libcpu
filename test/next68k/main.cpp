#include <libcpu.h>

#include "arch/m68k/libcpu_m68k.h"

#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "loader.h"

uint32_t
get32(FILE *f) {
	return fgetc(f)<<24 | fgetc(f)<<16 | fgetc(f)<<8 | fgetc(f);
}

uint32_t
load32(uint8_t * RAM, addr_t a) {
	return ntohl(*(uint32_t*)&RAM[a]);
}

void
store32(uint8_t * RAM, addr_t a, uint32_t b) {
	*(uint32_t*)&RAM[a] = htonl(b);
}

/*
 * The AmigaOS/Tripos Hunk format consists of any number of code/data hunks,
 * and each hunk comes with relocation information directly after it. This is
 * a list of offsets and hunknumbers: The offset in this hunk has to be
 * patched with the address of one specific hunk. Since relocation information
 * is not stored at the end of the file after all hunks, if the file is read
 * sequentially, the load addresses of the other hunks are not known yet,
 * so it is necesary to do two passes over the file.
 */
int
load_code(char *filename, uint8_t *RAM, int ramsize, addr_t *s, addr_t *e, addr_t *entry) {
	return 1;
}

#ifdef DEBUG_SYMBOLS
const char*
app_get_symbol_name(addr_t addr) {
	return "";
}
#endif

//////////////////////////////////////////////////////////////////////
// command line parsing helpers
//////////////////////////////////////////////////////////////////////
void tag_extra(cpu_t *cpu, char *entries) {
	addr_t entry;
	char* old_entries;

	while (entries && *entries) {
	/* just one for now */
		if (entries[0] == ',')
			entries++;
		if (!entries[0])
			break;
		old_entries = entries;
		entry = (addr_t)strtol(entries, &entries, 0);
		if (entries == old_entries) {
			printf("Error parsing entries!\n");
			exit(3);
		}
		cpu_tag(cpu, entry);
	}
}

void tag_extra_filename(cpu_t *cpu, char *filename) {
	FILE *fp;
	char *buf;
	size_t nbytes;

	fp = fopen(filename, "r");
	if (!fp) {
		perror("error opening tag file");
		exit(3);
	}
	fseek(fp, 0, SEEK_END);
	nbytes = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buf = (char*)malloc(nbytes + 1);
	nbytes = fread(buf, 1, nbytes, fp);
	buf[nbytes] = '\0';
	fclose(fp);

	while (nbytes && buf[nbytes - 1] == '\n')
		buf[--nbytes] = '\0';

	tag_extra(cpu, buf);
	free(buf);
}

void __attribute__((noinline))
breakpoint() {
asm("nop");
}

static void
debug_function(uint8_t *RAM, void *r) {
	reg_m68k_t *reg = (reg_m68k_t*)r;
//	printf("DEBUG: $%04X: A=$%02X X=$%02X Y=$%02X S=$%02X P=$%02X %02X/%02X\n", reg->pc, reg->a, reg->x, reg->y, reg->s, reg->p, RAM[0x33], RAM[0x34]);
//	{ int i; for (i=0x01F0; i<0x0200; i++) printf("%02X ", RAM[i]); printf("\n"); }
}

//////////////////////////////////////////////////////////////////////
int
main(int argc, char **argv) {
	char *executable;
	char *entries;
	cpu_t *cpu;
	uint8_t *RAM;

	int ramsize = 16*1024*1024;
	RAM = (uint8_t*)malloc(ramsize);
	if (!RAM) {
		fprintf(stderr, "Cannot allocate RAM\n");
		return 1;
	}

	cpu = cpu_new(CPU_ARCH_M68K);
	// cpu_set_flags_optimize(cpu, CPU_OPTIMIZE_ALL);
	cpu_set_flags_debug(cpu, CPU_DEBUG_NONE);
	cpu_set_ram(RAM);

	cpu_init(cpu);

/* parameter parsing */
	if (argc<2) {
		printf("Usage: %s executable [entries]\n", argv[0]);
		return 0;
	}

	executable = argv[1];
	if (argc>=3)
		entries = argv[2];
	else
		entries = 0;

/* load code */

	FILE *f;

	cpu->code_entry = (addr_t)LdrLoadMachO(executable, 0, (char*)RAM);
	cpu->code_start = 0;
	cpu->code_end = cpu->code_start + 16*1024*1024;  // fread(&RAM[cpu->code_start], 1, ramsize-cpu->code_start, f);

	if (!cpu->code_entry) {
		fprintf(stderr, "Cannot find Mach-O entry point\n");
		return 1;
	}

	cpu_tag(cpu, cpu->code_entry);

	if (entries && *entries == '@')
		tag_extra_filename(cpu, entries + 1);
	else
		tag_extra(cpu, entries); /* tag extra entry points from the command line */

#ifdef RET_OPTIMIZATION
	find_rets(RAM, cpu->code_start, cpu->code_end);
#endif

	printf("*** Executing... %lx\n", (unsigned long)cpu->code_entry);

#define PC (((reg_m68k_t*)cpu->reg)->pc)

	PC = cpu->code_entry;
	// S = 0xFF;

	for(;;) {
		breakpoint();
		int ret = cpu_run(cpu, debug_function);
		//printf("ret = %d\n", ret);
		switch (ret) {
			case JIT_RETURN_NOERR: /* JIT code wants us to end execution */
				break;
			case JIT_RETURN_FUNCNOTFOUND:
//				printf("LIB: $%04X: A=$%02X X=$%02X Y=$%02X S=$%02X P=$%02X\n", pc, a, x, y, s, p);

#if 0
				if (kernal_dispatch(RAM, &PC, &A, &X, &Y, &S, &P)) {
					// the runtime could handle it, so do an RTS
					PC = RAM[0x0100+(++(S))];
					PC |= (RAM[0x0100+(++(S))]<<8);
					PC++;
					continue;
				}
#endif
				
				// maybe it's a JMP in RAM: interpret it
				if (RAM[PC]==0x4C) {
					PC = RAM[PC+1] | RAM[PC+2]<<8;
					continue;
				}

				// bad :(
				printf("%s: error: $%08X not found!\n", __func__, PC);
				int i;
				printf("PC: ");
				for (i=0; i<16; i++)
					printf("%02X ", RAM[PC+i]);
				printf("\n");
				exit(1);
			default:
				printf("unknown return code: %d\n", ret);
		}
	}

	printf("done.\n");

	return 0;
}

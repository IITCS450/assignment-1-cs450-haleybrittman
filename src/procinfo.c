#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
static void usage(const char *a){fprintf(stderr,"Usage: %s <pid>\n",a); exit(1);}
static int isnum(const char*s){for(;*s;s++) if(!isdigit(*s)) return 0; return 1;}
int main(int c, char **v) {
	if (c != 2 || !isnum(v[1])) usage(v[0]);
	char stat_path[64], status_path[64], cmdline_path[64];
	snprintf(stat_path, sizeof(stat_path), "/proc/%s/stat", v[1]);
	snprintf(status_path, sizeof(status_path), "/proc/%s/status", v[1]);
	snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", v[1]);

		FILE *fstat = fopen(stat_path, "r");
	if (!fstat) {
		DIE("opening stat file");
	}
	int pid, ppid;
	char comm[256], state;
	unsigned long utime, stime;
	if (fscanf(fstat, "%d %255s %c %d", &pid, comm, &state, &ppid) != 4) {
		fclose(fstat);
		DIE_MSG("parsing stat file");
	}
	for (int i = 0; i < 10; ++i) {
		fscanf(fstat, "%*s");
	}
	if (fscanf(fstat, "%lu %lu", &utime, &stime) != 2) {
		fclose(fstat);
		DIE_MSG("reading utime/stime");
	}
	fclose(fstat);

	FILE *fstatus = fopen(status_path, "r");
	if (!fstatus) {
		DIE("opening status file");
	}
	char line[256];
	char vmrss[64] = "?";
	while (fgets(line, sizeof(line), fstatus)) {
		if (sscanf(line, "VmRSS: %63s", vmrss) == 1) {
			break;
		}
	}
	fclose(fstatus);

	FILE *fcmd = fopen(cmdline_path, "r");
	if (!fcmd) {
		DIE("opening cmdline file");
	}
	char cmdline[4096];
	size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, fcmd);
	fclose(fcmd);
	if (n == 0) {
		strcpy(cmdline, "?");
	} else {
		cmdline[n] = '\0';
		for (size_t i = 0; i < n; ++i) {
			if (cmdline[i] == '\0') cmdline[i] = ' ';
		}
	}

	long clk_tck = sysconf(_SC_CLK_TCK);
	double cpu_time = (utime + stime) / (double)clk_tck;

	printf("Process state: %c\n", state);
	printf("Parent PID: %d\n", ppid);
	printf("Command line: %s\n", cmdline);
	printf("CPU time (s): %.2f\n", cpu_time);
	printf("Resident memory (VmRSS): %s\n", vmrss);
	return 0;
}

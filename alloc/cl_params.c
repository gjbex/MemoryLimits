#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "cl_params.h"
#include "cl_params_aux.h"

#define MAX_CL_OPT_LEN 128
#define MAX_CL_LINE_LEN 1024

void initCL(Params *params) {
	params->maxMem = -1;
	params->incr = -1;
	params->sleep = 0;
}

void parseCL(Params *params, int *argc, char **argv[]) {
	char *argv_str;
	int i = 1;
	while (i < *argc) {
		 if (!strncmp((*argv)[i], "-?", 3)) {
			printHelpCL(stderr);
			finalizeCL(params);
			exit(EXIT_SUCCESS);
		}
		if (!strncmp((*argv)[i], "-maxMem", 8)) {
			shiftCL(&i, *argc, *argv);
			argv_str = (*argv)[i];
			if (!isLongCL(argv_str, 0)) {
				fprintf(stderr, "### error: invalid value for option '-maxMem' of type long\n");
				exit(EXIT_CL_INVALID_VALUE);
			}
			params->maxMem = atol(argv_str);
			i++;
			continue;
		}
		if (!strncmp((*argv)[i], "-incr", 6)) {
			shiftCL(&i, *argc, *argv);
			argv_str = (*argv)[i];
			if (!isLongCL(argv_str, 0)) {
				fprintf(stderr, "### error: invalid value for option '-incr' of type long\n");
				exit(EXIT_CL_INVALID_VALUE);
			}
			params->incr = atol(argv_str);
			i++;
			continue;
		}
		if (!strncmp((*argv)[i], "-sleep", 7)) {
			shiftCL(&i, *argc, *argv);
			argv_str = (*argv)[i];
			if (!isLongCL(argv_str, 0)) {
				fprintf(stderr, "### error: invalid value for option '-sleep' of type long\n");
				exit(EXIT_CL_INVALID_VALUE);
			}
			params->sleep = atol(argv_str);
			i++;
			continue;
		}
		break;
	}
	if (i > 1) {
		(*argv)[i - 1] = (*argv)[0];
		*argv = &((*argv)[i - 1]);
		*argc -= (i - 1);
	}
}

void parseFileCL(Params *params, char *fileName) {
	char line_str[MAX_CL_LINE_LEN];
	char argv_str[MAX_CL_LINE_LEN];
	FILE *fp;
	if (!(fp = fopen(fileName, "r"))) {
		fprintf(stderr, "### error: can not open file '%s'\n", fileName);
		exit(EXIT_CL_FILE_OPEN_FAIL);
	}
	while (fgets(line_str, MAX_CL_LINE_LEN, fp)) {
		if (isCommentCL(line_str)) continue;
		if (isEmptyLineCL(line_str)) continue;
		if (sscanf(line_str, "maxMem = %[^\n]", argv_str) == 1) {
			if (!isLongCL(argv_str, 0)) {
				fprintf(stderr, "### error: invalid value for option '-maxMem' of type long\n");
				exit(EXIT_CL_INVALID_VALUE);
			}
			params->maxMem = atol(argv_str);
			continue;
		}
		if (sscanf(line_str, "incr = %[^\n]", argv_str) == 1) {
			if (!isLongCL(argv_str, 0)) {
				fprintf(stderr, "### error: invalid value for option '-incr' of type long\n");
				exit(EXIT_CL_INVALID_VALUE);
			}
			params->incr = atol(argv_str);
			continue;
		}
		if (sscanf(line_str, "sleep = %[^\n]", argv_str) == 1) {
			if (!isLongCL(argv_str, 0)) {
				fprintf(stderr, "### error: invalid value for option '-sleep' of type long\n");
				exit(EXIT_CL_INVALID_VALUE);
			}
			params->sleep = atol(argv_str);
			continue;
		}
		fprintf(stderr, "### warning, line can not be parsed: '%s'\n", line_str);
	}
	fclose(fp);
}

void dumpCL(FILE *fp, char prefix[], Params *params) {
	fprintf(fp, "%smaxMem = %ld\n", prefix, params->maxMem);
	fprintf(fp, "%sincr = %ld\n", prefix, params->incr);
	fprintf(fp, "%ssleep = %ld\n", prefix, params->sleep);
}

void finalizeCL(Params *params) {
}

void printHelpCL(FILE *fp) {
	fprintf(fp, "  -maxMem <long integer>\n  -incr <long integer>\n  -sleep <long integer>\n  -?: print this message");
}

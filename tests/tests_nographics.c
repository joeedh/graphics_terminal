#include <stdio.h>
#include <string.h>
#include <float.h>

#include "../alloc.h"
#include "../stylecompiler/compiler.h"

int printlines(const char *buf, void *arg, errorprint_t printfunc) {
	int i, len = strlen(buf), lineno=1;
	int c=0;

	printfunc(arg, "%d: ", lineno);

	for (i=0; i<len; i++) {
		if (buf[i] == '\n') {
			lineno++;
			c += printfunc(arg, "\n%d: ", lineno);
		} else if (buf[i] == '\r' || buf[i] == '\b' || buf[i] == 0x7f || buf[i] == 27) {
			continue; //ignore common terminal control characters
		} else {
			c += printfunc(arg, "%c", buf[i]);
		}
	}

	printfunc(arg, "\n");

	return c;
}

const char *readfile(FILE *file) {
	size_t len;
	
	fseek(file, 0, SEEK_END);
	len = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *buf = MEM_malloc(len+1);

	fread(buf, len, 1, file);
	buf[len] = 0;

	return buf;
}

int main(int argc, char **argv) {
	FILE *input = NULL;

	fopen_s(&input, "teststyle.sl", "rb");

	if (!input) {
		fprintf(stderr, "Error: failed to open teststyle.sl!\n");
		return -1;
	}

	//read test file
	const char *buf = readfile(input);
	fclose(input);

	//print with line numbers
	printlines(buf, stdout, fprintf);

	//parse. . .
	int buflen=0;
	compilestyle(buf, strlen(buf), &buflen);

	system("PAUSE");
}

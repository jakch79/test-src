#include <string.h>

int main(int argc, char **argv) {
	char buf[4];
	char buf2[8] = "1234567";
	memcpy(buf, buf2, -100);
	return 1;
}

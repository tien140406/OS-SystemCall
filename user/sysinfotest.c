#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int
main(void)
{
	struct sysinfo st;

	if(sysinfo(&st) < 0){
		fprintf(2, "sysinfotest: sysinfo failed\n");
		exit(1);
	}

	printf("freemem: %d bytes\n", (int)st.freemem);
	printf("nproc: %d\n", (int)st.nproc);
	printf("nopenfiles: %d\n", (int)st.nopenfiles);

	if(st.freemem == 0 || st.nproc == 0){
		fprintf(2, "sysinfotest: invalid values\n");
		exit(1);
	}

	exit(0);
}

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int i = 0;

    // go through each string in argv
    // why am I skipping argv[0]?
    for(i = 1; i < argc; i++) {
        printf("arg %d: %s\n", i, argv[i]);
    }

    // let's make our own array of strings
    char *states[] = {
        "California", "Oregon",
        "Washington", "Texas", NULL
    };

	int num_states = sizeof(states);
	printf("number of states is %d\n", num_states);
	printf("%lu %lu %lu %lu\n", sizeof(states[0]), sizeof(states[1]), sizeof(states[2]), sizeof(states[3]));
    num_states = 5;

    for(i = 0; i < num_states; i++) {
        printf("state %d: %s\n", i, states[i]);
    }

    return 0;
}
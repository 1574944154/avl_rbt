#include <stdio.h>

char filename[] = "nums.txt";

int main(int argc, char *argv[])
{
    FILE *f = fopen(filename, "r");
    int num, i=0;

    while(fscanf(f, "%d", &num)!=EOF) 
    {
        printf("%d ", num);
        i ++;
    }
    printf("nums size is %d\n", i);

    return 0;
}


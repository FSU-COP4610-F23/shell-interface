#include <stdio.h>
#include <stdlib.h>

int main() {
  printf("USER : %s\n", getenv("USER"));
  printf("MACHINE : %s\n", getenv("MACHINE"));
  printf("PWD : %s\n", getenv("PWD"));

  return(0);
}


//beginning template for prompt

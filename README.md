# tma881-labs

## Relevant concepts

- **Stack allocation**.  We have learned in Assignment 0 that there are two
  types of allocated memory: Stack and heap allocated memory. One acquires
  stack allocated memory by using array declarations inside of functions, as
  opposed to global declarations of arrays.

  Memory allocated on the stack is limited in size, but tends to be faster.
  Moreover, stack allocated memory is thread local and therefore provides an
  opportunity to untangle the mutual impact of parallel threads on one another.
  Consequentially, it is an important consideration to employ stack allocated
  memory in the innermost iteration steps, i.e., the Newton iteration for an
  individual point on the complex plane.

  We plan to test this concept by comparing runtimes of variants of our program
  using stack and heap allocated memory in the innermost iteration steps.

***CONTINUE HERE WITH FUTHER CONCEPTS***

## Intended program layout

Per instructions the program naturally splits into two subtasks: The
computation of the Newton iteration and the writing of results to the two
output files. Each of them will be implemented in a separate function, that are
intended to be run as the main function of corresponding POSIX threads.

The computation of the Newton iteration can be further split up into
***INSERT***

As for the writing to file, we have identified ***INSERT*** independent
subtasks. ***INSERT***

***CONTINUE BY FURTHER SPLITTING UP THE TWO TASKS***

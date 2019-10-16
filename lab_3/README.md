# Assignment 3: OpenMP

## Relevant concepts

### OpenMP

### Syncronization in OpenMP

### Parsing strings

### Inlining??

### Cache locality, memory fragmentation??

### 

## Intended program layout

1. Solve example with a sequential program
    x Read file (reading):
        x Open file
        x Store each line in an array
        x Close file
    x For each line (parsing):
        x Create a coordinate struct containing three ints
        x Create a function for parsing a coordinate string to a coordinate struct
            x Function for parsing a single string as an int
        x Store struct in a large array
    x For all points (computation):
        x Compute the distance to all other points
            x Function for computing the distance between two points
        x Store result in an array
    x Print result
2. Parallelize sequential program
    - Parsing of coordinates
    - Computation of distances
        - Needs some mechanism for synchronizing writing to results array
3. Handle large files
    - read two chunks, 
    - calculate 
    - throw away the second chunk 
    - read next chunk 
    - calcuate first chunk with new chunk 
    - repeat

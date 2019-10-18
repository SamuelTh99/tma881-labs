# Assignment 3: OpenMP

## Relevant concepts

### OpenMP

- Loop collapsing?
- Synchronization (critical vs atomic vs reduction)
- Tasks (for reading next file chunk in parallel while computing distances for the previous one)
- Reduction of arrays
- Schedulers (we want to go for static, as each chunk will have approximately the same runtime)
    - Do we want to set chunk size manually?

### Parsing strings

### Inlining??

### Data locality, memory fragmentation??

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
    x Parsing of coordinates
    x Computation of distances
3. Handle large files
    - For each chunk:
        - Parse coordinates
        - Compute distances to all other coords in chunk
        - For each succeeding chunk:
            - Parse coordinates
            - Compute distances between all coords in first chunk and all coords in second chunk

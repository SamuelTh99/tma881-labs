__kernel void
    heat_diffusion(
        __global float* m,
        const uint n,
        const uint row_len,
        const float c
    )
{
    size_t i = get_global_id(0);
    
    float sum = 0;

    // left
    if (i % row_len != 0) {
        sum += m[i-1];
    }
    
    // right
    if (i % row_len != row_len - 1) {
        sum += m[i+1];
    }

    // up
    if (i >= row_len) {
        sum += m[i-row_len];
    }

    // down
    if (i + row_len < n) {
        sum += m[i+row_len];
    }
    
    m[i] = m[i] + c * (sum / 4 - m[i]);
}

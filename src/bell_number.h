#ifndef BELL_NUMBER_H
#define BELL_NUMBER_H

#include <vector>
#include <stdexcept>

/**
 * Computes the Bell number B(n), which represents the number of ways to partition a set of n elements.
 * 
 * The Bell number counts the number of different ways to partition a set into non-empty subsets.
 * For example:
 * - B(0) = 1 (empty set has one partition)
 * - B(1) = 1 (one element has one partition)
 * - B(2) = 2 (two elements can be partitioned as {{a,b}} or {{a},{b}})
 * - B(3) = 5 (three elements have five possible partitions)
 * 
 * @param n The number of elements in the set
 * @return The Bell number B(n)
 * @throws std::invalid_argument if n is negative
 */
unsigned long long compute_bell_number(int n) {
    if (n < 0) {
        throw std::invalid_argument("Number of elements cannot be negative");
    }
    
    if (n == 0 || n == 1) {
        return 1;
    }
    
    // Use Bell triangle method to compute Bell numbers
    std::vector<std::vector<unsigned long long>> bell_triangle(n);
    
    // Initialize the first row with B(1) = 1
    bell_triangle[0] = {1};
    
    // Fill the Bell triangle
    for (int i = 1; i < n; i++) {
        // Allocate space for the current row
        bell_triangle[i].resize(i + 1);
        
        // First number in the row is the last number of the previous row
        bell_triangle[i][0] = bell_triangle[i-1][i-1];
        
        // Fill the rest of the row
        for (int j = 1; j <= i; j++) {
            bell_triangle[i][j] = bell_triangle[i][j-1] + bell_triangle[i-1][j-1];
        }
    }
    
    // The Bell number B(n) is the last number in the last row
    return bell_triangle[n-1][n-1];
}

#endif // BELL_NUMBER_H


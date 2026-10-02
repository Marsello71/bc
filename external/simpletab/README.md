Source: https://github.com/zera/Nips_MT (src/framework/hashing.h), MIT license (see LICENSE).
Paper: Dahlgaard, Knudsen, Thorup, "Practical Hash Functions for Similarity Estimation
and Dimensionality Reduction", NIPS 2017. Simple tabulation: Patrascu & Thorup, JACM 2012.
File is kept unmodified. Class `simpletab` (line ~312) hashes a 32-bit key with 4 tables
of 256 x 32-bit entries; tables are filled from a degree-20 polynomial hash (class polyhash).

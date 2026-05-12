#pragma once

#define NN 312
#define MM 156
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
#define LM 0x7FFFFFFFULL /* Least significant 31 bits */

void init_genrand64(unsigned long long seed);
void init_by_array64(unsigned long long init_key[], unsigned long long key_length);
unsigned long long genrand64_int64(void);
/*
-------------------------------------------------------------------------------
jhash.h -- companion header for lookup3.c, by Bob Jenkins, May 2006, Public Domain.

lookup3.c itself ships upstream as a single file with no header (see
https://burtleburtle.net/bob/c/lookup3.c). This header only declares the
externally useful functions defined there so other translation units can
call them -- it adds no logic of its own.
-------------------------------------------------------------------------------
*/
#ifndef JHASH_H
#define JHASH_H

#include <cstddef>
#include <cstdint>

uint32_t hashword(const uint32_t *k, size_t length, uint32_t initval);
void     hashword2(const uint32_t *k, size_t length, uint32_t *pc, uint32_t *pb);
uint32_t hashlittle(const void *key, size_t length, uint32_t initval);
void     hashlittle2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);
uint32_t hashbig(const void *key, size_t length, uint32_t initval);

#endif // JHASH_H

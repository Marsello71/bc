/**
 * @file crc32c.h
 * @brief CRC-32C (Castagnoli, polynomial 0x1EDC6F41 / reflected 0x82F63B78).
 *
 * Portable table-driven reference implementation. Algorithm as specified in
 * RFC 3720 Appendix B.4 (iSCSI) and matching Mark Adler's public-domain
 * crc32c reference (https://stackoverflow.com/a/17646775). Software
 * slice-by-1 only; no hardware (SSE4.2 / ARMv8 CRC) path.
 *
 * Vendored unchanged. Added to this project by
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef CRC32C_H
#define CRC32C_H

#include <cstddef>
#include <cstdint>

/**
 * Running CRC-32C over buf[0..len).
 *
 * Pass 0 for @p crc on the first call. To process data in chunks, feed the
 * previous return value back in as @p crc. The standard 0xFFFFFFFF pre/post
 * conditioning is applied internally.
 */
uint32_t crc32c(uint32_t crc, const void *buf, size_t len);

#endif // CRC32C_H

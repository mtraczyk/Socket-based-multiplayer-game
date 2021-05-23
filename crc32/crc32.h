#ifndef CRC32_H
#define CRC32_H

#ifdef __cplusplus

extern "C" uint32_t crc32(const void *buf, size_t size);

#else

uint32_t crc32(const void *buf, size_t size);

#endif

#endif /* CRC32_H */

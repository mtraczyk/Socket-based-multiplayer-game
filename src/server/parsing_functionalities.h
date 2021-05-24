#ifndef PARSING_FUNCTIONALITIES_H
#define PARSING_FUNCTIONALITIES_H

#include <cstdint>
#include <vector>

// Sets program parameters, if some parameter is incorrect returns -1, otherwise returns 0.
int setProgramParameters(int argc, char *const argv[], uint16_t *, int64_t *,
                         uint8_t *, uint8_t *, uint16_t *, uint16_t *);

#endif /* PARSING_FUNCTIONALITIES_H */

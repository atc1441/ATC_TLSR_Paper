#ifndef MIJIA_BLE_LIBS_THIRD_PARTY_MICRO_ECC_UECC_CONFIG_H_
#define MIJIA_BLE_LIBS_THIRD_PARTY_MICRO_ECC_UECC_CONFIG_H_

/* Platform selection options.
If uECC_PLATFORM is not defined, the code will try to guess it based on compiler macros.
Possible values for uECC_PLATFORM are defined below: */
#define uECC_arch_other 0
#define uECC_x86        1
#define uECC_x86_64     2
#define uECC_arm        3
#define uECC_arm_thumb  4
#define uECC_arm_thumb2 5
#define uECC_arm64      6
#define uECC_avr        7

#include "types.h"

/* Choose your platform. */
//#define uECC_PLATFORM   uECC_arch_other

/* If desired, you can define uECC_WORD_SIZE as appropriate for your platform (1, 4, or 8 bytes).
If uECC_WORD_SIZE is not explicitly defined then it will be automatically set based on your
platform. */
//#define uECC_WORD_SIZE 4

/* Optimization level; trade speed for code size.
   Larger values produce code that is faster but larger.
   Currently supported values are 0 - 4; 0 is unusably slow for most applications.
   Optimization level 4 currently only has an effect ARM platforms where more than one
   curve is enabled. */
#ifndef uECC_OPTIMIZATION_LEVEL
    #define uECC_OPTIMIZATION_LEVEL 3
#endif

/* uECC_SQUARE_FUNC - If enabled (defined as nonzero), this will cause a specific function to be
used for (scalar) squaring instead of the generic multiplication function. This can make things
faster somewhat faster, but increases the code size. */
#ifndef uECC_SQUARE_FUNC
    #define uECC_SQUARE_FUNC 0
#endif

/* uECC_VLI_NATIVE_LITTLE_ENDIAN - If enabled (defined as nonzero), this will switch to native
little-endian format for *all* arrays passed in and out of the public API. This includes public
and private keys, shared secrets, signatures and message hashes.
Using this switch reduces the amount of call stack memory used by uECC, since less intermediate
translations are required.
Note that this will *only* work on native little-endian processors and it will treat the uint8_t
arrays passed into the public API as word arrays, therefore requiring the provided byte arrays
to be word aligned on architectures that do not support unaligned accesses.
IMPORTANT: Keys and signatures generated with uECC_VLI_NATIVE_LITTLE_ENDIAN=1 are incompatible
with keys and signatures generated with uECC_VLI_NATIVE_LITTLE_ENDIAN=0; all parties must use
the same endianness. */
#ifndef uECC_VLI_NATIVE_LITTLE_ENDIAN
    #define uECC_VLI_NATIVE_LITTLE_ENDIAN 1
#endif


/* Specifies whether compressed point format is supported.
   Set to 0 to disable point compression/decompression functions. */
#ifndef uECC_SUPPORT_COMPRESSED_POINT
    #define uECC_SUPPORT_COMPRESSED_POINT 0
#endif

#endif /* MIJIA_BLE_LIBS_THIRD_PARTY_MICRO_ECC_UECC_CONFIG_H_ */

#ifndef BITS_H
#define BITS_H

// Bit field and masking macros (partially defined in Arduino.h)
#ifndef bit

#define bit(flag)              (1UL << (flag))
#define bitRead(value, flag)   (((value) >> (flag)) & 0x01)
#define bitSet(value, flag)    ((value) |= bit(flag))
#define bitClear(value, flag)  ((value) &= ~bit(flag))
#define bitToggle(value, flag) ((value) ^= bit(flag))
#define bitWrite(value, flag, bitvalue) ((bitvalue) ? bitSet(value, flag) : bitClear(value, flag))

typedef unsigned char byte;
#endif

#define bitIsSet(value,flag)   (((value) & bit(flag)) != 0)
#define bitIsClear(value,flag) (((value) & bit(flag)) == 0)

#endif // BITS_H

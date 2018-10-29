#ifndef TMC26X_H
#define TMC26X_H

extern void tmc26x_init(void);
extern void setTMC26xRunCurrent(uint8_t level);
extern uint16_t getTMC26xStallGuard2Value(uint8_t cs_bit);

#endif

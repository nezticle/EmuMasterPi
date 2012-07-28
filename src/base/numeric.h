#ifndef NUMERIC_H
#define NUMERIC_H

#include <QtGlobal>

static inline quint8 bctToInt(quint bcd) {
	return (bcd & 0xF) | ((bcd >> 4) * 10);
}
static inline quint8 intToBcd(quint8 val) {
	return (val / 10) | ((val % 10) << 4);
}
static inline quint8 bcdIncrement(quint8 val) {
	val++;
	if ((val & 0x0F) >= 10)
		val += 0x10 - 10;
	return val;
}

#endif // NUMERIC_H

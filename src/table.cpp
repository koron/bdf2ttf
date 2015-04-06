#include <stdio.h>
#include <stdlib.h>
#include "table.h"

    void
bigfirst::addString(wchar_t* str)
{
    while (*str)
    {
	addByte((*str >> 8) & 0xff);
	addByte((*str >> 0) & 0xff);
	++str;
    }
}

    void
array::addString(char* str)
{
    while (*str)
	addByte(*str++);
}

    short
array::set(unsigned int s, unsigned char c)
{
    if (s >= len)
	return addByte(c);
    else
	data[s] = c;
    return true;
}

    void
array::clear()
{
    free(data);
    len = 0;
    data = NULL;
}

array::array()
{
    len = 0;
    data = NULL;
}

array::~array() {
    free(data);
}

     void
array::addArray(array& ary)
{
    int len = (int)ary.getLen();
    for (int i = 0; i < len; ++i)
	addByte(ary[i]);
}

    bool
array::extend()
{
    if ((len & 63) == 0)
    {
	unsigned char  *nd = (unsigned char  *)realloc(data, len + 64);
	if (!nd)
	    return false;
	else
	    data = nd;
    }
    return true;
}

    bool
array::addByte(unsigned char b)
{
    if (!extend())
	return false;
    data[len++] = b;
    return true;
}

    bool
array::insert(unsigned int s, unsigned char b)
{
    if (s >= len)
	return addByte(b);
    if (!extend())
	return false;
    for (unsigned int i = len; i > s; i--)
	data[i] = data[i - 1];
    data[s] = b;
    len++;
    return true;
}

    void
array::remove(unsigned int s)
{
    if (s >= len)
	return;
    remove(s, 1);
}

    void
array::remove(unsigned int start, unsigned int size)
{
    if (start + size >= len)
	return;
    len -= size;
    for (unsigned int i = start; i < len; i++)
	data[i] = data[i + size];
}

    unsigned long
table::setTableLen(unsigned long newlen)
{
#if 0
    /* Keep 4 byte alignment. */
    len = (newlen + 3) & ~3L;
#else
    len = newlen;
#endif
    return len;
}

    unsigned long
table::commitLen()
{
    return setTableLen(getLen());
}

    unsigned long
table::calcSum()
{
    int i, c;
    unsigned long l;
    unsigned char *t = (unsigned char *)getBuf();
    unsigned char *eot = t + getLen();
    chk = 0L;
    while (t < eot) {
	for (l = i = 0; i < 4; i++) {
	    c = (t < eot) ? *t++ : 0;
	    l = (l >> 8) + (c << 24);
	}
	chk += l;
    }
    return chk;
}

    void
table::write(FILE *fn)
{
    unsigned int maxlen = getLen();

    unsigned int i;
    for (i = 0; i < maxlen; ++i)
	putc((*this)[i],fn);

    // Padding with 0 (LOCA and HMTX)
    for (; i < len; ++i)
	putc(0,fn);
}


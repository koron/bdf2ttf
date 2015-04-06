#ifndef TABLE_H
#define TABLE_H

class array
{
    public:
	array();
	~array();

	void clear();

	bool addByte(unsigned char b);
	void addArray(array& ary);
	void addArray(array* ary) { addArray(*ary); };
	void addString(char* str);
	bool insert(unsigned int s, unsigned char b);
	short set(unsigned int s, unsigned char c);
	void remove(unsigned int s);
	void remove(unsigned int start, unsigned int size);

	unsigned char operator[](unsigned int s) {
	    return (s >= len) ? 0: data[s];
	}
	unsigned int	getLen() { return len; }
	unsigned char*	getBuf() { return data; }
    private:
	bool extend();
    private:
	unsigned int len;
	unsigned char *data;
};

class bigfirst : public array
{
    public:
	void addShort(unsigned short s) {
	    array::addByte((unsigned char)(s >> 8));
	    array::addByte((unsigned char)s);
	};
	void addLong(unsigned long s) {
	    addShort((unsigned short)(s >> 16));
	    addShort((unsigned short)s);
	};
	void addString(wchar_t* str);
};

class table: public bigfirst
{
    public:
	table(): bigfirst() {
	    chk = off = len = 0L;
	    mapTo = NULL;
	}
	unsigned long calcSum();
	void write(FILE *fn);
	unsigned long chk;
	unsigned long off;
    private:
	unsigned long len;
	table *mapTo;
    public:
	unsigned long commitLen();
	unsigned long setTableLen(unsigned long len);
	unsigned long getTableLen() { return len; }
	void setMapTable(table* value) { mapTo = value; }
	table* getMapTable() { return mapTo; }
};

#endif // TABLE_H

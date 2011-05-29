#ifndef __VECTORSTREAMBUF_H
#define __VECTORSTREAMBUF_H

#include <streambuf>
#include <vector>

class VectorStreambuf : public std::streambuf
{
	std::vector<char> buffer;
    std::vector<char>::iterator readIter;

public:
	VectorStreambuf()
	{
		//Nothing
	}//constructor

	virtual ~VectorStreambuf()
	{
		//Nothing
	}//destructor

    void prepareForInput()
    {
        readIter = buffer.begin();
    }//prepareForInput

protected:
	virtual int xsputn(const char *s, int n) 
	{ 
		buffer.insert(buffer.end(), s, s+n);
		return n;
	}//xsputn

	virtual int overflow(int ch)
	{
		if (std::char_traits<char>::eof() != ch) {
			buffer.push_back(ch);
		}//if
		
		return ch;
	}//overflow

	virtual int xsgetn(char *s, int n) 
	{
		int distance = static_cast<int>(std::distance(readIter, buffer.end()));
		n = std::min(n, distance);

        std::vector<char>::iterator endIter = readIter;
        std::advance(endIter, n);
        std::copy(readIter, endIter, s);
        std::advance(readIter, n);

		return n;
	}//xsgetn

	virtual int uflow()
	{
		char retVal = *readIter;
		++readIter;
		return std::char_traits<char>::to_int_type(retVal);
	}//uflow

	virtual int underflow()
	{
		char retVal = *readIter;
		++readIter;
		return std::char_traits<char>::to_int_type(retVal);
	}//underflow

    virtual int pbackfail(int c = std::char_traits<char>::eof())
	{
		if (c != std::char_traits<char>::eof()) {
			return std::char_traits<char>::not_eof(c);
		} else {
			return std::char_traits<char>::eof();
		}//if
	}//pbackfail

	virtual std::streamsize showmanyc()
	{ 
		std::streamsize retVal = static_cast<std::streamsize>(std::distance(readIter, buffer.end()));
		return retVal;
	}//showmanc
};//VectorStreambuf

#endif

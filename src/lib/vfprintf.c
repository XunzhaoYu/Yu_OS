#include "string.h"
#include "assert.h"
#include "common.h"

/*
 * use putchar_func to print a string
 *   @putchar_func is a function pointer to print a character
 *   @format is the control format string (e.g. "%d + %d = %d")
 *   @data is the address of the first variable argument
 * please implement it.
 */
int
vfprintf(void (*putchar_func)(char), const char *format, void **data) {
	const char *s;
	char *x = (char*)data;
	int count = 0;
	for (s = format; *s; s ++) 
	{
		if(*s == '%')
		{
			s ++;
			if(*s == 'd')
			{
				x = itoa((int)*data,10);
				data = data + 1;
			}
			else if(*s == 'x')
			{
				x = itoa((int)*data,16);
				data = data + 1;		
			}
			else if(*s == 's')
			{
				x = (char *)*data;
				data = data + 1;			
			}
			else if(*s == 'c')
			{	
				x = memset(x,(int)*data, (size_t)1) ;
				data = data + 1;		
			}
			else
			{
				putchar_func('%');
				count++;
				x = memset(x,*s, (size_t)1);
			}
			while(*x)
			{
				putchar_func(*x);
				count++;
				x++;
			}
		}
		else
		{
			putchar_func(*s);
			count++;
		}	
	}
	count --;
	return count;
}



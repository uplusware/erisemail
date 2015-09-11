#ifndef _BASE64_H_
#define _BASE64_H_

#define BASE64_ENCODE_OUTPUT_MAX_LEN(L) ( ((L) <  4 ) ? (4 + 1) : ((L)/3*4 + ((L)%3 ? 4 : 0) + 1) )
#define BASE64_DECODE_OUTPUT_MAX_LEN(L) ( ((L) <= 4 ) ? (4 + 1) : ((L)/4*3 + 1) )

class CBase64
{
protected:
	static void _init_codes_(char codes[])
	{
		int i;
		for (i = 0; i < 256; i++) codes[i] = -1;
		for (i = 'A'; i <= 'Z'; i++) codes[i] = i - 'A';
		for (i = 'a'; i <= 'z'; i++) codes[i] = 26 + i - 'a';
		for (i = '0'; i <= '9'; i++) codes[i] = 52 + i - '0';
		codes['+'] = 62;
		codes['/'] = 63;
	}

public:
	CBase64()
	{
	}
	virtual~ CBase64()
	{
	}
	
	static int Decode(char data[], int length, char* out, int * outlen)
	{
		int value, ix;
		int shift = 0;   
		int accum = 0;   
		int index = 0;
		int len;
		char codes[256];
		len = ((length + 3) / 4) * 3;
		if (length>0 && data[length-1] == '=') --len;
		if (length>1 && data[length-2] == '=') --len;
		_init_codes_(codes);
		for (ix=0; ix<length; ix++)
		{
			value = codes[ data[ix] & 0xFF ];
			if ( value >= 0 )
			{
				accum <<= 6;
				shift += 6;
				accum |= value;
				if ( shift >= 8 )
				{ 
					shift -= 8;
					out[index] = ((accum >> shift) & 0xff);
					index++;
					if(index >= *outlen)
					{
						return -1;
					}
				}
			}
		}
		*outlen = index;
		return 0;
	}
	
	static int Encode(char data[], int length, char* out, int * outlen)
	{
		const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
		
		int i, index, val;
		int quad, trip; 
		for (i=0, index=0; i<length; i+=3, index+=4)
		{
			if(index >= *outlen)
			{
				return -1;
			}
			quad = 0;
			trip = 0;
			val = (0xFF & (int) data[i]);
			val <<= 8;
			if ((i+1) < length) 
			{
				val |= (0xFF & (int) data[i+1]);
				trip = 1;
			}
			val <<= 8;
			if ((i+2) < length) 
			{
				val |= (0xFF & (int) data[i+2]);
				quad = 1;
			}
			out[index+3] = alphabet[(quad ? (val & 0x3F): 64)];
			val >>= 6;
			out[index+2] = alphabet[(trip ? (val & 0x3F): 64)];
			val >>= 6;
			out[index+1] = alphabet[val & 0x3F];
			val >>= 6;
			out[index+0] = alphabet[val & 0x3F];
		}
		out[index] = '\0';
		*outlen = index;
		return 0;
	}

};

#endif	/*_BASE64_H_*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ecc.h"

/* Reed-solomon command-line tool by Vitaly "_Vi" Shukela; 2012 */

/*
  Usage:
    1. Download http://sourceforge.net/projects/rscode/
    2. Tune NPAR in ecc.h to the value you want
    3. Build rscode, put this source file to it's directory;
    3. gcc -o reedsolomon reedsolomon.c -L. -lecc
    
  Encoding:
    Start "reedsolomon encode" and feed it lines to encode.
    It should write hex-encoded bytes to stdout (your input + NPAR bytes)
    If "reedsolomon encode --hex" it will read hex strings instead of raw data
    
    $ reedsolomon encode
    ABC
    414243234EB33A
    ABCD
    4142434420877B3B
    
    $ reedsolomon encode --hex
    FF00FF
    FF00FF885F7DD5

    
  Decoding:
    Start "reedsolomon decode" and feed the hex lines (minimum 2*NPAR+2 hex characters per line)
    Use "__" instead of a byte for erasures.
    The tool wil output "G ", "C ", "B " (good, corrected, uncorrected respectively) followed by your data
    "reedsolomon decode --hex" will print hex strings
    
    $ reedsolomon decode
    4142434420877B3B
    G ABCD
    41FF434420877B3B
    C ABCD
    41FF434420877FFF 
    B A?CD
    41FF43442087____
    C ABCD

    $ reedsolomon decode --hex
    41FF43442087____
    C 41424344

    There are pre-built versions of the tool for static i386 and for Android ARM at
    http://vi-server.org/pub/
*/

unsigned char codeword[256];
unsigned char msg[256-NPAR];
int erasures[256];
int nerasures;

int n;

int readhex(unsigned char* out, int maxn) {
    char buf[516];
    int i;
    char* ret = fgets(buf, maxn*2, stdin);
    printf("ret: %c\n", *ret);
    if (!ret) return 0;
    
    const char* eos = strchr(buf, '\n');
    if (eos) n = (eos - buf)/2; else n=maxn;
        
    nerasures = 0;
    
    for (i=0; i<n; ++i) {
        char tmp = buf[i*2+2];
        buf[i*2+2]=0;
        if(buf[i*2]=='_') { 
            erasures[nerasures++]=i; 
            out[i] = 0;
        } else {
            out[i] = strtol(buf+i*2, NULL, 16);
        }
        buf[i*2+2]=tmp;
    }
    
    return n;
}

void writehex(unsigned char* out, int n) {
    int i;
    for (i=0; i<n; ++i) {
        fprintf(stdout, "%02X", out[i]);
    }
}

int main(int argc, char* argv[]) {
	   if (argc < 2) {
        fprintf(stderr, "Usage: reedsolomon {encode|decode} [--hex]\n");
        fprintf(stderr, "/* Reed Solomon Coding for glyphs\n"
            " * Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009\n"
            " * This software library is licensed under terms of the GNU GENERAL\n"
            " * PUBLIC LICENSE v3+ */\n");
        fprintf(stdout, "NPAR=%d\n", NPAR);
        return 1;
    }
    
    int hexmode = 0;
    if (argc >= 3 && !strcmp(argv[2], "--hex")) hexmode = 1;

    if(getenv("DEBUG")) DEBUG=1;
    
    initialize_ecc ();
   
    int i;
    
    if (!strcmp(argv[1], "encode")) {  
        for (;;) {
            if (!hexmode) {
                char* ret = fgets((char*)msg, sizeof(msg), stdin);
                if (!ret) break;
                
                const unsigned char* eos = (unsigned char*) strchr((char*)msg, '\n');
                if (eos) n = eos - msg; else n=sizeof(msg)-1;
            } else {
                n = readhex(msg, sizeof(msg));
                if (!n) break;
            }
            
            encode_data(msg, n, codeword);
            
            writehex(codeword, n+NPAR);
            printf("\n");
            fflush(stdout);
        }
        
    } else
    if (!strcmp(argv[1], "decode")) {
         for(;;) {
            n = readhex(codeword, sizeof(codeword)) - NPAR;
            
            if (n==-NPAR) return 0;
            if (n<0) {continue; }
            
            for(i=0; i<nerasures; ++i) {
                erasures[i] = n+NPAR - erasures[i]-1;
            }
            
            decode_data(codeword, n+NPAR);

			int s = check_syndrome();

			if(s) {
				 correct_errors_erasures(codeword, n+NPAR, nerasures, erasures);
			}
            
            if (hexmode) {
                writehex(codeword, n);
            } else {
                fwrite(codeword, 1, n, stdout);
                fflush(stdout);
            }
            fflush(stdout);
        }
    }
 
    return 0;
}

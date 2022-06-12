#include <stdio.h>    // for I/O functions
#include <stdlib.h>   // for exit()
#include <string.h>   // for string functions
#include <time.h>     // for time functions

int i, j;

unsigned short temp, inst, addr;
char buf[300];

FILE *infile;
FILE *outfile;
char c, *p, letter;

unsigned short mca[65536]; // 2^16 RAM, 2 bytes word
int mcaindex;

unsigned short start;
int gotstart;

unsigned short Gadd[1000];
char *Gptr[1000];
int Gindex;

unsigned short Eadd[1000];
char *Eptr[1000];
int Eindex;

unsigned short eadd[1000];
char * eptr[1000];
int eindex;

unsigned short Aadd[1000];
int Amodadd[1000];
int Aindex;

unsigned short Vadd[1000];
char *Vptr[1000];
int Vindex;

int main(int argc,char *argv[])
{
   // Take and check command line arguments
   if (argc < 2)
   {
        printf("Wrong number of command line arguments\n");
        printf("Usage: l <obj module name1> <obj module name2> ... \n");
        exit(1);
   }

   // For each module, store header entries into tables with adjusted
   // addresses and store machine code in mca - machine code array.
   for (i = 1; i < argc; i++)
   {
      infile = fopen(argv[i], "rb");
      if (!infile)
      {
         printf("Cannot open %s\n", argv[i]);
         exit(1);
      }
      printf("Linking %s\n", argv[i]);
      letter = fgetc(infile);
      if (letter != 'o')
      {
         printf("Not a linkable file\n");
         exit(1);
      }
      while (1)
      {
         letter = fgetc(infile);
         if (letter == 'C')
            break;
         else
         if (letter == 'S')
         {
            if (fread(&addr, 2, 1, infile) != 1) // addr unsigned short
            {
               printf("Invalid S entry\n");
               exit(1);
            }
            if (gotstart)
            {
               printf("More than one entry point\n");
               exit(1);
            }
            gotstart = 1;                   // indicate S entry processed
            start = addr + mcaindex;        // save adjusted address
         }
         else
         if (letter == 'G')
         {
            if (fread(&addr, 2, 1, infile) != 1)
            {
               printf("Invalid G entry\n");
               exit(1);
            }
            Gadd[Gindex] = addr + mcaindex; // save adjusted address
            j = 0;
            do                              // get string in G entry
            {
               letter = fgetc(infile);
               buf[j++] = letter;
            } while (letter != '\0');
            j = 0;
            while (j < Gindex)     // check for multiple definitions
            {
               if (!strcmp(buf, Gptr[j]))
               {
                  printf("Multiple defs of global var %s\n", buf);
                  exit(1);
               }
               else
                  j++;
            }
            Gptr[Gindex++] = strdup(buf);   // save string
         }
         else
         if (letter == 'E')
         {
            /* The next value after 'E' is address relative to start of the module
             * use this value to calculate adjusted address then save the address
             * to Eadd[] and for coresponding label save in Eptr[]
             */
            if(fread(&addr, 2, 1, infile) != 1) {
                printf("invalid E entry\n");
                exit(1);
            }
            Eadd[Eindex] = addr + mcaindex;

            // the next value after address is label which save to buf[]
            j = 0; 
            do
            {
               letter = fgetc(infile);
               buf[j++] = letter;
            } while (letter != '\0');

            // compare all char pointer in Eptr[] to lable in buf
            // if there is a similar label in Eptr we have multible definition for 1 label
            // if buf == Eptr[j] then strcmp return 0 which is false, 
            // !0 make the condition of "if" true.
            j = 0;
            while(j < Eindex) {  
                if(!strcmp(buf, Eptr[j])) {  
                  printf("Multiple defs of E var %s\n", buf);
                  exit(1);
                }    
                j++;
            }

            // no duplicate so save label to Eptr[]
            // strdup allocate duplicate label in heap return its pointer
            // increase Eindex after save, so the next E entry 
            // will go to the next location in Eadd and Eptr
            Eptr[Eindex++] = strdup(buf);
         }
         else
         if (letter == 'e')
         {
            if(fread(&addr, 2, 1, infile) != 1) {
                printf("invalid e entry\n");
                exit(1);
            }
            eadd[eindex] = addr + mcaindex;
            j = 0; 
            do
            {
               letter = fgetc(infile);
               buf[j++] = letter;
            } while (letter != '\0');
            j = 0;
            while( j < eindex) {
                if (!strcmp(buf, eptr[j])) {
                    printf("Multiple defs of e var %s\n", buf);
                    exit(1);
                }
                j++;
            }
            eptr[eindex++] = strdup(buf);
         }
         else
         if (letter == 'V')
         {
            if (fread(&addr, 2, 1, infile) != 1){
                printf("invalid V entry\n");
                exit(1);
            }
            Vadd[Vindex] = addr + mcaindex;
            j = 0; 
            do
            {
               letter = fgetc(infile);
               buf[j++] = letter;
            } while (letter != '\0');
            j = 0;
            while(j < Vindex) {
                if(!strcmp(buf, Vptr[j])) {
                    printf("Multiple defs of V var %s\n", buf);
                    exit(1);
                }
                j++;
            }
            Vptr[Vindex++] = strdup(buf);
         }
         else
         if (letter == 'A')
         {
            if (fread(&addr, 2, 1, infile) != 1){
                printf("invalid A entry\n");
                exit(1);
            }
            Aadd[Aindex] = addr + mcaindex;
            Amodadd[Aindex] = mcaindex;
            Aindex++;
         }
         else
         {
            printf("Invalid header entry %c in %s\n", letter, argv[i]);
            exit(1);
         }
      }

      // add machine code to machine code array
      while(fread(&inst, 2, 1, infile))
      {
         mca[mcaindex++] = inst;
      }
      fclose(infile);
   }

   //================================================================
   //Adjust external references

   // handle E references
   for (i = 0; i < Eindex; i++)
   {
      for (j = 0; j < Gindex; j++)
         if(!strcmp(Eptr[i], Gptr[j]))
            break;
      if (j >= Gindex)
      {
         printf("%s is an undefined external reference", Eptr[i]);
         exit(1);
      }
      mca[Eadd[i]] = (mca[Eadd[i]] & 0xf800) |
                     ((mca[Eadd[i]] + Gadd[j] - Eadd[i] - 1) & 0x07ff);
   }
   // handle e entries
   for (i = 0; i < eindex; i++)   // for each e entry at index i
   {
      // use j to go through G table
      for(j = 0; j < Gindex; j++) {
         if (!strcmp(eptr[i], Gptr[j]))         
            break;   
      }

      // j out of G table range mean no match
      if (j >= Gindex) {
         printf("%s is an undefined external reference", eptr[i]);
         exit(1);
      }

      // there is a match, resolve reference
      //
      // mca[eadd[i]] is instruction that has bits field to adjust
      // mca[eadd[i]] & 0xfe00 will zero out 9 left bits
      // & 0x1ff will zero out 7 right bits.
      //
      // to get offset subtract address of G label to address of e label + 1
      // address of Glabel is Gadd[j], address of e is eadd[i]
      //
      // we need to add mca[eadd[j]] to the offset because the pc-offset field of this
      // instruction might contain label offset. 
      mca[eadd[i]] = (mca[eadd[i]] & 0xfe00) |
                     ((mca[eadd[i]] + Gadd[j] - eadd[i] - 1) & 0x01ff);  
   }

   // handle V entries
   for (i = 0; i < Vindex; i++)
   {
      for (j = 0; j < Gindex; j++) {
         if (!strcmp(Vptr[i], Gptr[j]))
             break;
      }
      if (j >= Gindex) { 
         printf("%s is an undefined external reference", eptr[i]);
         exit(1);
      }
      // mca[Vadd[i]] = (mca[Vadd[i]] & 0x0000) | Gadd[j];
      mca[Vadd[i]] += Gadd[j];
   }

   //================================================================
   //Handle A entries

   for (i = 0; i < Aindex; i++)
      mca[Aadd[i]] += Amodadd[i];

   //================================================================
   //Write out executable file

   outfile = fopen("link.e", "wb");
   if (!outfile)
   {
      printf("Cannot open output file link.e\n");
      exit(1);
   }

   // Write out file signature
   fwrite("o", 1, 1, outfile);
   
   printf("Creating executable file link.e\n");
   // Write out start entry if there is one
   if (gotstart)
   {
      fwrite("S", 1, 1, outfile);
      fwrite(&start, 2, 1, outfile);
   }
   // Write out G entries
   for (i = 0; i < Gindex; i++)
   {
      fwrite("G", 1, 1, outfile);
      fwrite(Gadd + i, 2, 1, outfile);
      fprintf(outfile, "%s", Gptr[i]);
      fwrite("", 1, 1, outfile);
   }
   // Write out V entries as A entries
   for (i = 0; i < Vindex; i++)
   {
      fwrite("A", 1, 1, outfile);
      fwrite(Vadd + i, 2, 1, outfile);
   }
   // Write out A entries
   for (i = 0; i < Aindex; i++)
   {                        
      fwrite("A", 1, 1, outfile);
      fwrite(Aadd + i, 2, 1, outfile);
   }
   // Terminate header
   fwrite("C", 1, 1, outfile);

   // Write out code
   for (i = 0; i < mcaindex; i++)
   {
      fwrite(mca + i, 2, 1, outfile);
   }
   fclose(outfile);
}

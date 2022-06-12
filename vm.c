#include <stdio.h>  
#include <stdlib.h>
#include <time.h> 

FILE *infile;
short r[8], mem[65536], offset6, imm5, imm9, pcoffset9, pcoffset11, 
      regsave1, regsave2;
unsigned short ir, pc, opcode, code, dr, sr, sr1, sr2, baser, bit5, bit11,
               trapvec, n, z, c, v;
char letter;

// Helper function to find sign of value after ALU operations
void setnz(short r)
{
   n = z = 0;
   if (r < 0)    // is result negative?
      n = 1;     // set n flag
   else
   if (r == 0)   // is result zero?
      z = 1;     // set z flag
}

// Helper fuction to decide if over flow occured, set c flag and v flag accordingly
void setcv(short sum, short x, short y)
{
   v = c = 0;
   if (x >= 0 && y >= 0)   // if both non-negative, then no carry
      c = 0;
   else
   if (x < 0 && y < 0)     // if both negative, then carry
      c = 1;
   else
   if (sum >= 0)           // if signs differ and sum non-neg, then carry
      c = 1;
   else                    // if signs differ and sum neg, then no carry
      c = 0;
   // if signs differ then no overflow
   if ((x < 0 && y >= 0) || (x >= 0 && y < 0))
      v = 0;
   else
   // if signs the same and sum has different sign, then overflow
   if ((sum < 0 && x >= 0) || (sum >= 0 && x < 0))
      v = 1;
   else
      v = 0;
}

int main(int argc, char *argv[])
{
   //Check for correct files input
   if (argc != 2)
   {
       printf("Wrong number of command line arguments\n");
       printf("Usage: i1 <input filename>\n");
       exit(1);
   }

   infile = fopen(argv[1], "rb"); // open file in binary mode
   if (!infile)
   {
      printf("Cannot open input file %s\n", argv[1]);
      exit(1);
   }

   fread(&letter, 1, 1, infile);  // test for and discard get file sig
   if (letter != 'o')
   {
      printf("%s not an lcc file", argv[1]);
      exit(1);
   }
   fread(&letter, 1, 1, infile);  // test for and discard 'C'
   if (letter != 'C')
   {
      printf("Missing C header entry in %s\n", argv[1]);
      exit(1);
   }

   fread(mem, 1, sizeof(mem), infile); // read machine code into mem

   while (1)
   {
      // fetch instruction, load it into ir, and increment pc
      ir = mem[pc++];                    

      // isolate the fields of the instruction in the ir
      opcode = ir >> 12;                  // get opcode
      pcoffset9 = ir << 7;                // left justify pcoffset9 field
      pcoffset9 = imm9 = pcoffset9 >> 7;  // sign extend and rt justify
      pcoffset11 = ir << 5;               // left justify pcoffset11 field
      pcoffset11 = pcoffset11 >> 5;       // sign extend and rt justify
      imm5 = ir << 11;                    // left justify imm5 field
      imm5 = imm5 >> 11;                  // sign extend andd rt justify
      offset6 = ir << 10;                 // left justify offset6 field
      offset6 = offset6 >> 10;            // sign extend and rt justify
      trapvec = opcode = ir & 0x1f;       // get trapvec and eopcode fields
      code = dr = sr = (ir << 4) >> 13;   // get code/dr/sr and rt justify
      sr1 = baser = (ir & 0x01c0) >> 6;   // get sr1/baser and rt justify
      sr2 = ir & 0x0007;                  // get third reg field
      bit5 = ir & 0x0010;                 // get bit 5
      bit11 = ir & 0x0800;                // get bit 11

      // Now we have approriate field of an instruction, just need to
      // decode (i.e., determine) and execute instruction just fetched
      switch (opcode)
      {
         case 0:                          // branch instructions
            switch(code)
            {
               case 0: if (z == 1)             // brz
                          pc = pc + pcoffset9;
                       break;
               case 1: if (z == 0)             // brnz
                          pc = pc + pcoffset9;
                       break;
               case 2: if (n == 1)             // brn
                          pc = pc + pcoffset9;
                       break;
               case 3: if (n == z)             // brp
                          pc = pc + pcoffset9;
                       break;
               case 4: if (n != v)             // brlt
                          pc = pc + pcoffset9;
                       break;
               case 5: if (n == v && z == 0)   // brgt
                          pc = pc + pcoffset9;
                       break;
               case 6: if (c == 1)             // brc
                          pc = pc + pcoffset9;
                       break;
               case 7: pc = pc + pcoffset9;    // br
                       break;
            }                                                   
            break;
         case 1:                               // add
            if (bit5)
            {
               regsave1 = r[sr1];
               r[dr] = regsave1 + imm5;
               // set c, v flags
               setcv(r[dr], regsave1, imm5);
            }
            else
            {
               regsave1 = r[sr1]; regsave2 = r[sr2];
               r[dr] = regsave1 + regsave2;
               // set c, v flags
               setcv(r[dr], regsave1, regsave2);
            }
            // set n, z flags
            setnz(r[dr]);
            break;
         case 2:                          // ld
            r[dr] = mem[pc + pcoffset9];
            break;
         case 3:                          // st
            mem[pc + pcoffset9] = r[sr];
            break;
         case 4:                          // bl     
            r[7] = pc;
            pc = pc + pcoffset11;
            break;
         case 5:                          // and
            if (bit5)
            {
                regsave1 = r[sr1];
                r[dr] = regsave1 & imm5;
            }
            else
            {
                regsave1 = r[sr1]; regsave2 = r[sr2];
                r[dr] = regsave1 & regsave2;
            }
            setnz(r[dr]);
            break;
         case 6:                          // ldr
            r[dr] = mem[baser + offset6];
            break;
         case 7:                          // str
            mem[baser + offset6] = sr;
            break;
         case 8:                          // cmp    
            if (bit5)
            {
                regsave1 = r[sr1];
                setcv(regsave1 - imm5, regsave1, imm5);
            }
            else
            {
                regsave1 = r[sr1]; regsave2 = r[sr2];
                setcv(regsave1 - regsave2, regsave1, regsave2);
            }
            setnz(regsave1 - regsave2);
            break;
         case 9:                          // not
            // ~ is the not operator in C
            r[dr] = ~r[sr1];
            // set n, z flags
            setnz(r[dr]);
            break;
         case 10:                                            
           switch(imm5) 
           {
               case 0:                    // push  
                   mem[--r[6]] = sr;
                   break;
               case 1:                    // pop
                   dr = mem[r[6]++];
                   break;
           }
           break;
         case 11:                         // sub
            if (bit5)
             {
                regsave1 = r[sr1];
                r[dr] = regsave1 - imm5;
                // set c, v flags
                setcv(r[dr], regsave1, imm5);
             }
             else
             {
                regsave1 = r[sr1]; regsave2 = r[sr2];
                r[dr] = regsave1 - regsave2;
                // set c, v flags
                setcv(r[dr], regsave1, regsave2);
             }
             // set n, z flags
             setnz(r[dr]);
             break;
         case 12:                         // jmp/ret
            pc = r[baser] + offset6;                     
            break;
         case 13:                         // mvi
            dr = imm9;
            break;
         case 14:                         // lea
            r[dr] = pc + pcoffset9;
            break;
         case 15:                         // trap
            if (trapvec == 0x00)             // halt
               exit(0);
            else
            if (trapvec == 0x01)             // nl
               printf("%c",'\n');
            else
            if (trapvec == 0x02)             // dout
               printf("%d",r[sr]);                
            break;
      }     // end of switch
   }        // end of while
}

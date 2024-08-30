//-----------------------------LICENSE NOTICE------------------------------------
//  Copyright (C) 2024 Néstor Gracia (https://github.com/nestornillo)
//  Copyright (C) 2024 CPCtelera's Telegram Group (@FranGallegoBR)
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//-------------------------------------------------------------------------------

//  Amdivgen 1.1
//  Amstrad division function generator
//
//   This program generates routines to calculate the division (integer
//  quotient) of a number contained in the A register by a constant value.
//
//   It also can generate routines for multiplicating the value contained
//  in A by a fraction num1/num2, where num2 is a power of 2, and num1
//  is less or equal to num2.
//
//   All resulting functions take the input value from the A register and
//  return the result also in A register. Some of the generated functions
//  use the B register.
//
//   The resulting functions are optimised trying to get maximum speed on
//  an Amstrad CPC (without using lookup tables), but, as they are
//  automatically generated, the obtained code is not the fastest
//  possible in some cases.
//
//   This program has been tested in a 64 bits linux using gcc
//  ("gcc amdivgen.c -o amdivgen").
//



//  Amdivgen 1.1
//  Generador de rutinas de división para Amstrad.
//
//
//   Este programa genera rutinas para calcular la división (parte entera)
//  de un número contenido en el registro A entre un valor constante.
//
//   También permite generar rutinas para la multiplicación del valor
//  contenido en A por una fracción num1/num2, donde num2 es una potencia
//  de 2, y num1 es menor o igual a num2.
//
//   Todas las rutinas generadas reciben el valor de entrada en el registro A y
//  devuelven el resultado de la división en el registro A. Algunas de
//  las rutinas generadas usan el registro B.
//
//   Las funciones resultantes están optimizadas intentando obtener la mayor
//  velocidad posible en un Amstrad (sin usar una tabla con todos los
//  resultados precalculados), pero, al estar generadas automáticamente,
//  el código obtenido no es el más rápido posible en algunos casos.
//
//   Este programa se ha probado en un linux 64 bits usando gcc
//  ("gcc amdivgen.c -o amdivgen").
//




#include "stdio.h"
#include "stdlib.h"

#define MAXPOWER2 24

enum asmLines{ _ld_ba =1, _rra, _srl_a, _add_b, _ret, _and_fc, _and_f8, _and_f0, _and_e0, _and_c0, _and_80, _rlca, _rrca, _rla, _and_01, _and_03, _and_07, _and_0f,_xor_a};
enum paramregistersUsed{ _only_use_a, _destroys_b};
int resultLines[50];
int numResultLines=0;
int resultLinesTemp[50];
int numResultLinesTemp=0;
int sizeResult=0;
int speedResult=0;


/////////////////////
// PRINTING FUNCTIONS
/////////////////////

// Prints help
void printHelp(void){
  printf("\n    Amdivgen 1.1         Amstrad division function generator\n\n");
  printf(" This program generates routines for dividing an 8-bit number by\n");
  printf(" a constant value.\n");
  printf("\nUsage:\n");
  printf(" amdivgen num\n");
  printf("       Creates a function that divides the number contained in A register\n");
  printf("       by the number passed as a parameter\n");
  printf("       i.e.:   amdivgen 3.1416     creates routine for A = A / 3.1416\n\n");
  printf(" amdivgen num1 num2\n");
  printf("       Creates a routine which multiplies the input value by the fraction\n");
  printf("       num1/num2  (where num2 is a power of 2, and num1<=num2)\n");
  printf("       i.e.:   amdivgen 17 256     creates routine for A = A * (17/256)\n\n");
  printf(" amdivgen 0 num\n");
  printf("       Shows approximations used to create the division function by a\n");
  printf("       given number\n");
  printf("       i.e.:   amdivgen 0 10       shows approximations to A = A / 10\n\n");
  printf(" amdivgen -num\n");
  printf("       Creates a division function by num, using always approximation\n");
  printf("       by a fraction\n");
  printf("       i.e.:   amdivgen -121       creates routine for A = A / 121\n\n");
}

// Prints an array showing the powers of two that composes a given number
void showPowers(int num) {
  int powtwo;
  powtwo=1<<MAXPOWER2;
  for (int j=0;j<=MAXPOWER2;j++) {
    if (num>powtwo-1) {
      printf("%d ",MAXPOWER2-j);
      num-=powtwo;
    }
    powtwo=powtwo/2;
  }
}

// Prints diferent fraction multiplication approximations for a given divider.
// Shows test ('OK' or first number that fails) and decompositions.
void showInfo(float n) {
  int dividerBase2;
  int div;
  int value;
  int correct;
  printf (" Amdivgen 1.1         Approximations to 1/%g\n",n);
  printf ("     approx        test      decomposition into powers of 2\n",n);
  for (dividerBase2=0;dividerBase2<=MAXPOWER2;dividerBase2++) {
    div=1<<dividerBase2;
    if (n==div) printf("       1/%-8g   OK    %8d:%-2d        1:0\n",n,div,dividerBase2);
    value=(div/n)+1;
    printf ("%8d/%-8d ",value,div);
    correct=1;
    for (int j=0;j<256;j++){
      if ( (int)((j*1000)/(n*1000)) != ((value*j)/div) ) {
        correct=0;
        printf("Err:%-3d ", j);
        j=256;
      }
    }
    if (correct==1) printf("  OK    ");
    printf("%8d:%-2d %8d:",div,dividerBase2,value);
    showPowers(value);
    printf("\n");
  }
}

// Header printing functions
void printDivisionBy(float num){
  printf(";;\n");
  printf(";; Division by %g\n", num);
  printf(";;\n;; Returns the integer quotient of dividing\n", num);
  printf(";; the input value by %g \n",num);
  printf(";;\n;;   A = A / %g \n;;\n",num);
}
void printMultiplicationBy(float num,int divisor){
  printf(";;\n");
  printf(";; Multiplication by fraction %d/%d\n",(int)num,divisor);
  printf(";;\n;; Returns the integer part of multiplying\n", num);
  printf(";; the input value by the fraction %d/%d\n",(int)num,divisor);
  printf(";;\n;;   A = A * ( %d / %d )\n;;\n",(int)num,divisor);
}
void printCredits(void){
  printf(";;\n;; Function created with Amdivgen 1.1\n");
  printf(";; https://github.com/nestornillo/amdivgen\n;;\n");
}
void printHeaderNumberBigger85Smaller128(float num) {
  int doublenum;
  doublenum=num*2;
  if (num!=(int)num) doublenum++;
  printDivisionBy(num);
  printf(";;   Input: A register\n;;  Output: A register\n;;\n");
  printf(";;         Size: 12 bytes\n");
  printf(";; Average time: %0.2f microseconds\n",(doublenum/(float)256)+10);
  printf(";;   Worst time: 11 microseconds\n");
  printf(";;    Best time: 10 microseconds\n");
  printCredits();
  printf("division_by_%g::\n", num);
}
void printHeader(float num,int size,int speed,int registers,int divisor) {
  if (divisor!=0) {
    printMultiplicationBy(num,divisor);
    printf(";;   Input: A register\n;;  Output: A register\n");
    if (registers==_destroys_b) printf(";;\n;; Destroys B register\n");
    printf(";;\n;; %d bytes / %d microseconds\n",size,speed);
    printCredits();
    printf("fraction_%d_%d::\n", (int)num,divisor);
  }
  else {
    printDivisionBy(num);
    printf(";;   Input: A register\n;;  Output: A register\n");
    if (registers==_destroys_b) printf(";;\n;; Destroys B register\n");
    printf(";;\n;; %d bytes / %d microseconds\n",size,speed);
    printCredits();
    printf("division_by_%g::\n", num);
  }
}

// Code printing function
void printlines(void) {
  for (int i=0;i<numResultLines;i++) {
    switch(resultLines[i]){
      case _ld_ba: printf("ld b,a    ; [1]\n"); break;
      case _rra:   printf("rra       ; [1]\n"); break;
      case _srl_a: printf("srl a     ; [2]\n"); break;
      case _add_b: printf("add b     ; [1]\n"); break;
      case _ret:   printf("ret       ; [3]\n"); break;
      case _and_fc:printf("and #0xFC ; [2]\n"); break;
      case _and_f8:printf("and #0xF8 ; [2]\n"); break;
      case _and_f0:printf("and #0xF0 ; [2]\n"); break;
      case _and_e0:printf("and #0xE0 ; [2]\n"); break;
      case _and_c0:printf("and #0xC0 ; [2]\n"); break;
      case _and_80:printf("and #0x80 ; [2]\n"); break;
      case _rlca:  printf("rlca      ; [1]\n"); break;
      case _rrca:  printf("rrca      ; [1]\n"); break;
      case _rla:   printf("rla       ; [1]\n"); break;
      case _and_01:printf("and #0x01 ; [2]\n"); break;
      case _and_03:printf("and #0x03 ; [2]\n"); break;
      case _and_07:printf("and #0x07 ; [2]\n"); break;
      case _and_0f:printf("and #0x0F ; [2]\n"); break;
      case _xor_a :printf("xor a     ; [1]\n"); break;
      default:  printf(";;---ERROR printlines---\n");
    }
  }
}


////////////////////////////
// CODE GENERATION FUNCTIONS
////////////////////////////

// Add one instruction to the code
void addLine(int asmInstruction) {
  resultLines[numResultLines]=asmInstruction;
  numResultLines++;
}

// Add one instruction to temp code
void addLineTemp(int asmInstruction) {
  resultLinesTemp[numResultLinesTemp]=asmInstruction;
  numResultLinesTemp++;
}


// measure size and speed of generated code
void measureCode(void) {
  sizeResult=0;
  speedResult=0;
  for (int i=0;i<numResultLines;i++) {
    switch(resultLines[i]){
      case _ret:
      sizeResult+=1; speedResult+=3; break;
      case _ld_ba: case _rra: case _add_b: case _rlca: case _rrca: case _rla: case _xor_a:
      sizeResult+=1; speedResult+=1; break;
      case _srl_a: case _and_fc: case _and_f8: case _and_f0: case _and_e0: case _and_c0: case _and_80: case _and_01: case _and_03: case _and_07: case _and_0f:
      sizeResult+=2; speedResult+=2; break;
      default:
      printf(";;---ERROR measureCode---\n");
    }
  }
}

// Optimize function by changing consecutive srla to a more compact equivalent form
void optimizeCode(void) {
  int srlaInARow;
  int modnextline;
  numResultLinesTemp=0;
  for (int i=0;i<numResultLines;i++) {
    srlaInARow=0;
    modnextline=0;
    switch(resultLines[i]) {
      case _rra:
        for (int j=i;resultLines[j+1]==_srl_a;j++) {
          srlaInARow++; // count following srlas
        }
        switch(srlaInARow) { // optimizations for 1 rra + n srla
          case 4: addLineTemp(_rla);addLineTemp(_rla);addLineTemp(_rla);addLineTemp(_rla);addLineTemp(_and_0f);modnextline=4;break;
          case 5: addLineTemp(_rla);addLineTemp(_rla);addLineTemp(_rla);addLineTemp(_and_07);modnextline=5;break;
          case 6: addLineTemp(_rla);addLineTemp(_rla);addLineTemp(_and_03);modnextline=6;break;
          case 7: addLineTemp(_rla);addLineTemp(_and_01);modnextline=7;break;
          default:
            addLineTemp(resultLines[i]);
        }
        break;
      case _srl_a:
        for (int j=i;resultLines[j]==_srl_a;j++) {
          srlaInARow++;// count srlas
        }
        switch(srlaInARow) { // optimizations for n srla
          case 3: addLineTemp(_and_f8);addLineTemp(_rrca);addLineTemp(_rrca);addLineTemp(_rrca);modnextline=2;break;
          case 4: addLineTemp(_and_f0);addLineTemp(_rrca);addLineTemp(_rrca);addLineTemp(_rrca);addLineTemp(_rrca);modnextline=3;break;
          case 5: addLineTemp(_and_e0);addLineTemp(_rlca);addLineTemp(_rlca);addLineTemp(_rlca);modnextline=4;break;
          case 6: addLineTemp(_and_c0);addLineTemp(_rlca);addLineTemp(_rlca);modnextline=5;break;
          case 7: addLineTemp(_and_80);addLineTemp(_rlca);modnextline=6;break;
          case 8: addLineTemp(_xor_a);modnextline=7;break;
          default:
            addLineTemp(resultLines[i]);
        }
        break;
      default:
        addLineTemp(resultLines[i]);
    }
    i+=modnextline; // skip substituted lines
  }
  numResultLines=0; // reset result counter
  for (int i=0;i<numResultLinesTemp;i++) {
    addLine(resultLinesTemp[i]); // copy temp to result
  }
}

// Create code for a multiplication by a fraction
void generateCode(float num,int i,int div,int divpow) {
  int difference;
  int arrrayPowersOf2[MAXPOWER2+1];
  int numpowers=0;
  int powtwo;
  powtwo=1<<MAXPOWER2;
  for (int j=0;j<MAXPOWER2+1;j++) {
    if (i>powtwo-1) {
      arrrayPowersOf2[numpowers]=MAXPOWER2-j; // fill arrrayPowersOf2[] with the decomposition of i into powers of two
      numpowers++;
      i-=powtwo;
    }
    powtwo=powtwo/2;
  }
  for (int j=numpowers-1;j>0;j--) {
    difference=arrrayPowersOf2[j-1]-arrrayPowersOf2[j];
    if ( ( (j==numpowers-1)&&(difference>7) ) || (difference>8) )  numpowers=j; // discard smaller powers if difference is too big
  }
  if ((divpow-arrrayPowersOf2[0])>8) {
    addLine(_xor_a); // if divider is too big, result is always zero
    numpowers=1;
  }
  else {
    if (numpowers>1) addLine(_ld_ba); // store input in b if it's needed later (if there's more than 1 power of two)
    for (int j=numpowers-1;j+1>0;j--) {
      if (j!=0){
        difference=arrrayPowersOf2[j-1]-arrrayPowersOf2[j];
        if(j==numpowers-1) {
          addLine(_srl_a);
          difference--;
        }
        else {
          addLine(_rra); // rotate right using carry of previous 'add b' as bit 7
          difference--;
        }
        while (difference>0) {
          addLine(_srl_a);  // add srlas until next power
          difference--;
        }
        addLine(_add_b);  // add input value
      }
    }
    difference=divpow-arrrayPowersOf2[0]-1;
    if (numpowers!=1) {
      addLine(_rra);
    }
    else {
      if (!((numpowers==1)&&(divpow==arrrayPowersOf2[0])))  addLine(_srl_a);
    }
    while (difference>0) {
      addLine(_srl_a); // add srlas according to remaining difference
      difference--;
    }
  }
  addLine(_ret);
  optimizeCode();
  measureCode();
  if (numpowers>1) {
    printHeader(num,sizeResult,speedResult,_destroys_b,div);
  }
  else {
    printHeader(num,sizeResult,speedResult,_only_use_a,div);
  }
  printlines();
}


// Find a fraction multiplication equivalent to the desired division
void findApproximation(float i) {
  int dividerBase2;
  int div;
  int value;
  int correct;
  for (dividerBase2=0;dividerBase2<=MAXPOWER2;dividerBase2++) {
    div=1<<dividerBase2;
    value=(div/i)+1;
    correct=1;
    for (int j=0;j<256;j++) { // test approximation for all 256 numbers
      if ( (int)((j*1000)/(i*1000)) != ((value*j)/div) ) {
        correct=0;  // if an error found mark as incorrect
        j=256;
      }
    }
    if (i==div) {  //if number is a power of two
       generateCode(div,1,0,dividerBase2);
       dividerBase2=200;  // exit for loop
    }
    if (correct==1) { // approximation works
       generateCode(i,value,0,dividerBase2);
       dividerBase2=200;
    }
  }
}

// if a number is a power of two, returns exponent+1. If not, returns 0.
int isPowerOf2(int num) {
  int div,dividerBase2;
  for (dividerBase2=0;dividerBase2<=MAXPOWER2;dividerBase2++) {
    div=1<<dividerBase2;
    if (num==div) return dividerBase2+1;
  }
  return 0;
}

// Creates a division function for numbers bigger than 128 up to 255
void numberBigger128UpTo255(float num) {
  int integernum;
  integernum=num;
  if (integernum!=num) integernum++;  // adjust for non-integers
  printHeader(num,5,7,_only_use_a,0);
  printf("cp #%-3d   ; [2]\n", integernum);
  printf("sbc a     ; [1]\n");
  printf("inc a     ; [1]\n");
  printf("ret       ; [3]\n");
}

// Creates a division function for numbers bigger than 85 and smaller than 128
void numberBigger85Smaller128(float num) {
  int integernum;
  int doublenum;
  integernum=num;
  if (integernum!=num) integernum++;
  doublenum=num*2;
  if (doublenum!=num*2) doublenum++;
  printHeaderNumberBigger85Smaller128(num);
  printf("cp #%-3d   ; [2]\n", doublenum);
  printf("jr nc,more_than_%-3d ; [2/3]\n",doublenum-1);
  printf("cp #%-3d   ; [2]\n", integernum);
  printf("sbc a     ; [1]\n");
  printf("inc a     ; [1]\n");
  printf("ret       ; [3]\n");
  printf("more_than_%d:\n",doublenum-1);
  printf("ld a,#2   ; [2]\n");
  printf("ret       ; [3]\n");
}

// Creates a division function for numbers bigger than 64 up to 85
void numberBigger64UpTo85(float num) {
  int integernum;
  int doublenum;
  int triplenum;
  integernum=num;
  if (num!=integernum) integernum++;
  doublenum=num*2;
  if (doublenum!=num*2) doublenum++;
  triplenum=num*3;
  if (triplenum!=num*3) triplenum++;
  printHeader(num,15,12,_only_use_a,0);
  printf("cp #%-3d   ; [2]\n", doublenum);
  printf("jr c,less_than_%-3d ; [2/3]\n",doublenum);
  printf("cp #%-3d   ; [2]\n", triplenum);
  printf("sbc a     ; [1]\n");
  printf("add #3    ; [2]\n");
  printf("ret       ; [3]\n");
  printf("less_than_%d:\n",doublenum);
  printf("cp #%-3d   ; [2]\n", integernum);
  printf("sbc a     ; [1]\n");
  printf("inc a     ; [1]\n");
  printf("ret       ; [3]\n");
}

// Main function
int main(int argc, char **argv) {
  float num;
  float param1;
  float param2;
  if (argc==1) {
    printHelp();
    return 1;
  }
  param1=atof(argv[1]);
  if (argc>2) {
    param2=atof(argv[2]);
    if (param1==0) {
      if (param2<1) {
        printf("Divisor must be greater than or equal to 1.\n");
        return 1;
      }
      showInfo(param2);
    }
    else {
      num=isPowerOf2(param2);
      if (num==0) {
        printf("Divisor must be a power of 2.\n");
        return 1;
      }
      if (param1>param2) {
        printf("Divisor must be greater than or equal to dividend.\n");
        return 1;
      }
      if ((param1!=atoi(argv[1]))||(param1<0)) {
        printf("Dividend must be a positive integer.\n");
        return 1;
      }
      generateCode(param1,param1,param2,num-1);
    }
  }
  else {
    num=param1;
    if (num<=-1){
      findApproximation(-num);
    }
    else if (num<1) {
      printf("Divisor must be greater than or equal to 1.\n");
      return 1;
    }
    else if ((num>128)&&(num<=255)) {
      numberBigger128UpTo255(num);
    }
    else if ((num>85)&&(num<128)) {
      numberBigger85Smaller128(num);
    }
    else if ((num>64)&&(num<=85)) {
      numberBigger64UpTo85(num);
    }
    else {
      findApproximation(num);}
  }
  return 0;
}

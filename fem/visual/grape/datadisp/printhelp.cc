//**************************************************************
//  (C) written and directecd by Robert Kloefkorn 
//**************************************************************

#ifndef __PRINTHELP_CC__
#define __PRINTHELP_CC__

static void print_help(const char *funcName)
{
  printf("usage: %s <i_start> <i_end> -i increment", funcName);
  printf(" [-h] [-help] [-p path] ");
  printf("[-m grid] [-s df] [-v vecdf] [[-s drv] [-v drdv]]\n");

  printf("%s reads a sequence of %ddgrids with discretefunctions\n",            
   funcName, DUNE_PROBLEM_DIM);
  printf("      and displays all data with GRAPE\n");
  printf("options:\n");
  printf("   -h or -help: display this help\n");
  printf("   -f: use one fixed grid (from non adaptive simulations)\n");
  printf("   -p path: read data from path 'path'\n");
  printf("   -m grid: basename of grids is 'grid'; default: 'grid'\n");
  printf("   -s df:  read discrete function with basename 'df'\n");

  printf("Example\n");
  printf("%s 0 10 -i 5 -s u_h \n",funcName);
  printf("  reads grid grid0000000000 with scalar function u_h0000000000 and\n");
  printf("  then grid0000000005 with u_h0000000005 and\n");
  printf("  and finally grid0000000010 with u_h0000000010\n");
  exit(0);
}
#endif

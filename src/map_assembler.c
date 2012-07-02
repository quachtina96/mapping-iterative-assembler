/* $Id: map_assembler.c 1051 2008-02-11 15:38:53Z green $ */
#include <stdio.h>
#include <stdlib.h>
#include "map_align.h"
#include "map_alignment.h"
#include "io.h"

/*
    void * save_malloc(size_t size){
        void * tmp = malloc(size);
        if (tmp == NULL)
            fprintf( stderr, "Some memory allokation failed. Exiting");

        return tmp;
    }
*/

void help( void ) {
  printf( "ma -M <maln input file>\n" );
  printf( "   -c <consensus code> \n" );
  printf( "   -f <output format>\n" );
  printf( "   -R <REGION_START:REGION_END>\n" );
  printf( "   -I <ID to assign to assembly sequence>\n" );
  printf( "ma reports information from a maln assembly file as generated by mia\n" );
  printf( "How the assembly calls each base can be determined by the\n" );
  printf( "consensus code. 1 = highest, positive aggregate score base (if any)\n" );
  printf( "                2 = highest aggregate score base if it is %d higher\n",
	  MIN_SC_DIFF_CONS );
  printf( "                    than second highest\n" );
  printf( "The output format can be specified through -f as one of the following.\n" );
  printf( "More complete descriptions of these output formats is below,\n" );
  printf( "under FORMATS\n" );
  printf( "1 => clustalw\n" );
  printf( "2 => line format; one line each for consensus, reference\n" );
  printf( "     and coverage\n" );
  printf( "3 => column format; one line per base, one column for consensus,\n" );
  printf( "     reference, and coverage; includes header with summary info\n" );
  printf( "4 => columns description of all assembly data for positions that differ\n" );
  printf( "     between consensus and CURRENT reference sequence (see FORMATS, below)\n" );
  printf( "41 => same as above, but for ALL positions\n" );
  printf( "5 => fasta format output of assembled sequence only\n" );
  printf( "6 => show all fragments in a region specified by -R\n" );
  printf( " -C Color format 6 output -> don't pipe this output to file!\n" );
  printf( "7 => ACE\n\n" ); 
  printf( "\nFORMATS (option f):\n" );
  printf( "1 => clustalw\n" );
  printf( "2 => line format; first line is \"Consensus, chrM, coverage:\"\n" );
  printf( "      second line is the entire, assembled, aligned consensus sequence\n" );
  printf( "      third line is the entire aligned reference sequence to which the\n" );
  printf( "      consensus is aligned\n" );
  printf( "      fourth line is the sequence coverage at each position in a space-\n" );
  printf( "      separated list of integers\n" );
  printf( "3 => column format; header shows summary statistics; table has one row\n" );
  printf( "      per position; columns are described in the output\n" );
  printf( "4 => alternative column format with one row per base that differes between\n" );
  printf( "      the consensus assembly and the reference of this iteration. \n" );
  printf( "      Note that in the FINAL iteration reference and consensus are equal! \n" );
  printf( "      So there won't be any output. Each row has the following\n" );
  printf( "      columns: (1)position on reference; 0-based coordinates, (2) reference\n" );
  printf( "      base, (3)consensus assembly base, (4)coverage, (5)A's, (6)C's, (7)G's,\n" );
  printf( "      (8)T's, (9)gaps; columns 5 through 9 should add up to column 4\n" );
  printf( "      (10) aggregate score for A, (11) aggregate score for C\n" );
  printf( "      (12) aggregate score for G, (13) aggregate score for T\n" );
  printf( "41=> same as above, but for every position\n" );
  printf( "5 => fasta format using ID \"Consensus\" for the assembly\n" );
  printf( "6 => region; shows the reference sequence, the consensus sequence, and then\n" );
  printf( "      all assembled fragments in a region specified by option -R\n" );
  printf( "61=> same as above, but in multi-fasta format for viewing in Bioedit, e.g.\n" );
  printf( "     (also requires a region as specified by the option -R\n" );
  printf( "7 => ACE format\n" ); 
}

void parse_region( char* reg_str, int* reg_start, int* reg_end ) {
  int tmp;
  sscanf( reg_str, "%d:%d", reg_start, reg_end );
  /* Flip em around if l-user is so stupid that he puts the bigger one first */
  if ( *reg_start > *reg_end ) {
    tmp = *reg_start;
    *reg_start = *reg_end;
    *reg_end = *reg_start;
  }
}

int main( int argc, char* argv[] ) {
  char mafn[MAX_FN_LEN+1];
  char ma_in_fn[MAX_FN_LEN+1];
  char assign_id[MAX_ID_LEN+1];
  unsigned int any_arg;
  int id_assigned = 0; // Boolean, set to true if -I is given
  int cons_scheme;
  int out_ma   = 0;
  int in_ma    = 0;
  int no_dups  = 0; // allow duplicate ids by default - the user knows what he's doing
  int out_format = 1;
  int reg_start  = 90;
  int reg_end    = 109;
  int in_color   = 0;  // Output f6 format colored -> bad when you want to pipe it into a file
  MapAlignmentP maln;
  IDsListP rest_ids_list, // the IDs in the -i argument, if any, will go here
    used_ids_list;        // the IDs seen thusfar; just for this list,
                          // the segment character is tacked onto the end
  int ich, cons_scheme_def, ids_rest;
  double score_int, score_slo;
  extern char* optarg;
  cons_scheme_def = 1;
  ids_rest = 0; // Boolean set to no => no IDs restriction set (yet)
  /* Get input options */
  any_arg = 0;
  cons_scheme = cons_scheme_def;
  score_int = -1.0; // Set the score intercept to -1 => not specified (yet)
  score_slo = -1.0; // Set the score intercept to -1 => not specified (yet)
  while( (ich=getopt( argc, argv, "I:c:i:f:R:s:m:M:Cb:s:d" )) != -1 ) {
    switch(ich) {
    case 'h' :
      help();
      exit( 0 );
      any_arg = 1;
      break;
    case 'I' :
      strcpy( assign_id, optarg );
      id_assigned = 1;
      break;
    case 'c' :
      cons_scheme = atoi( optarg );
      any_arg = 1;
      break;
    case 'i' :
      rest_ids_list = parse_ids( optarg );
      ids_rest = 1;
      any_arg = 1;
      break;
    case 'f' :
      out_format = atoi( optarg );
      any_arg = 1;
      break;
    case 'R' :
      parse_region( optarg, &reg_start, &reg_end );
      any_arg = 1;
      break;
    case 's' :
      score_slo = atof( optarg );
      any_arg = 1;
      break;
    case 'b' :
      score_int = atof( optarg );
      any_arg = 1;
      break;
    case 'C' : 
      in_color = 1;
      break;
    case 'm' :
      strcpy( mafn, optarg );
      out_ma  = 1;
      any_arg = 1;
      break;
    case 'M' :
      strcpy( ma_in_fn, optarg );
      in_ma   = 1;
      any_arg = 1;
      break;
    case 'd' :
      no_dups = 1;
      used_ids_list = init_ids_list();
      any_arg = 1;
      break;
    default :
      help();
      any_arg = 1;
      exit( 0 );
    }
  }
  if ( !any_arg || 
       ( (score_slo == -1) && (score_int != -1) ) ||
       ( (score_slo != -1) && (score_int == -1) ) ) {
    help();
    exit( 0 );
  }

  /* Initialize maln, either from specified input file or 
     brand new */
  if ( in_ma ) {
    maln = read_ma( ma_in_fn );
  }

  else {
    help();
    exit( 0 );
  }

  /* Set the maln->cons_code to something reasonable */
  maln->cons_code = cons_scheme;

  /* Now input from all sources has been dealt with, we turn our 
     attention to output...*/
  sort_aln_frags( maln );

  /* If an ID to be assigned to the assembly was given, then assign it now */
  if ( id_assigned ) {
    strcpy( maln->ref->id, assign_id );
  }

  if ( (out_format == 6) ||
       (out_format == 61) ) {
    print_region( maln, reg_start, reg_end, out_format, in_color );
  }
  else {
    show_consensus( maln, out_format );
  }
  
  if (out_format == 7){
	  ace_output(maln);
  }

  /* Write MapAlignment output to a file */
  if ( out_ma ) {
    write_ma( mafn, maln );
  }

  exit( 0 );
}


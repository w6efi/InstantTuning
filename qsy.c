#include <stdio.h>
#include <complex.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>

/*
qsy matchbox antenna
  program enters look waiting for a
  frequency selection from user
  It then reports best match at that frequency
*/

#define RI(x) (double)creal(x),(double)cimag(x)

// magnitude of complex number
static double cmplx_mag
(
complex double const z
)
{
  return sqrt(pow(creal(z),2) + pow(cimag(z),2));
}

// prints a 2x2 complex matrix
static void show2x2
(
double complex A[2*2]
)
{
  fprintf(stdout,"[(%f,%f)\t(%f,%f)]\n",RI(A[0]),RI(A[2]));
  fprintf(stdout,"[(%f,%f)\t(%f,%f)]\n",RI(A[1]),RI(A[3]));
}

static int const NF = 801; // # frequencies
static int const NF_A = 2006; // # lines in antenna file (s1p)
static int const NC = 11;  // # possible settings cap
static int const NL = 12;  //                     ind
static double const Z0 = 50;
static double const R_1 = 1.0;

// converts one-port impedance Z11 to S11 parameter
static void z11_to_s
(
double complex* z11
)
{
  double complex const Z11 = *z11;
  *z11 = (Z11-Z0)/(Z11+Z0);
};

// one port S11 to Z11
static void s11_to_z
(
double complex* z11
)
{
  double complex const S11 = *z11;
  *z11 = Z0*(1.0 + S11)/(1.0 - S11);
};

// terminate port ONE with load gamma_term (gamma_term is the S11 of the input to the termination) ... (bkw means backward)
static void s22_to_s11_bkw
(
const double complex S[2*2],
const double complex gamma_term,
double complex *gamma
)
{
  *gamma = S[3] + (S[2]*S[1]*gamma_term)/(R_1 - S[0]*gamma_term);
};

// terminate port TWO with load gamma_term ...
static void s22_to_s11_fwd
(
const double complex S[2*2],
const double complex gamma_term,
double complex *gamma
)
{
  *gamma = S[0] + (S[2]*S[1]*gamma_term)/(R_1 - S[3]*gamma_term);
};

// same as complex mag, probably redundant
static double mag
(
double complex z
)
{
  return sqrt(pow(creal(z),2) + pow(cimag(z),2));
}

/*
return true if  a <= x <= b
*/
static int between
(
double const a,
double const b,
double const x
)
{
  assert( a < b );
  if( (a <= x) && (x <= b) )
    return 1;
  return 0;
}

// an S-parameter at a specific frequency
// perform linear interpolation of a list of such entries
// should be ant_struct
typedef struct
{
  double freq; // MHz...
  complex double spar;
} fs_pair;

// find the frequencies in the antenna sweep that bracket the user-supplied frequency
// and then linearly interpolate the impedance
static void interpolate
(
fs_pair* list,
int const n,
double const freq, // MHz
complex double* s_out
)
{
  int j;
  assert( n >= 2 );
  if( freq < list[0].freq )
  {
    fprintf(stderr,"bound error for interpolation %f < %f\n",freq,list[0].freq);
    exit(0);
  }
  if( freq > list[n-1].freq )
  {
    fprintf(stderr,"bound error for interpolation %f > %f\n",freq,list[n-1].freq);
    exit(0);
  }

  j = 0;
  while( 0 == between(list[j].freq,list[j+1].freq,freq) )
  {
    j++;
    assert( j <= n-2 );
  }
  assert( 0 <= j );
  assert( j <= n-2 );
  double const delta = list[j+1].freq-list[j].freq;
  assert( delta > 0 );
  double const sigma = (freq-list[j].freq)/delta;
  assert( 0 <= sigma );
  assert( sigma <= 1 );
  *s_out = (1-sigma)*list[j].spar + sigma*list[j+1].spar;
}

/*
Data structure to capture one measurement
i.e., one setting of L and C, then NF (number of frequencies swept) two-port S params
*/
typedef struct
{
  char code[9]; // e.g., 07 2.00.   character label for this combo of knob settings
  fs_pair* S11; // capturing all [NF] of the measurements
  fs_pair* S21;
  fs_pair* S12;
  fs_pair* S22;
} one_setting;

int main
(
int const argc,
char* argv[]
)
{
  double const version = 1.0;
  if( argc != 3 )
  {
    fprintf(stdout,"qsy matchbox_file antenna_file\n");
    exit(0);
  }

  double const fa = 0.8; // set by VNA sweep
  double const fb = 32;
  
  // // read in S-parameter data for the matchbox (personality file).
  // // mbox is an array of "one_setting"s of size num C settings x num of L settings
  // // inside each "one_setting" is 4 arrays (of S11, S12, S21, S22) each of length [NF]

  // open the personality file, and set up the memory to accept the data
  FILE* fp_mbox = fopen(argv[1],"r");
  assert( fp_mbox );
  one_setting* mbox = malloc(NC*NL*sizeof(one_setting));
  assert( mbox );  // when malloc fails it returns 0 so assert will fail

  // Read in the matchbox personality file into memory
  // Reminder -- for each combo of settings, have 4*NF complex numbers
  for( int jL = 0; jL < NL; jL++ )
  {
    for( int jC = 0; jC < NC; jC++ )
    {
      double rp, ip;
      double skip;
      char buf[256];

      mbox[jC*NL+jL].S11 = (fs_pair*)malloc(NF*sizeof(fs_pair));
      mbox[jC*NL+jL].S21 = (fs_pair*)malloc(NF*sizeof(fs_pair));
      mbox[jC*NL+jL].S12 = (fs_pair*)malloc(NF*sizeof(fs_pair));
      mbox[jC*NL+jL].S22 = (fs_pair*)malloc(NF*sizeof(fs_pair)); 
      fgets(buf,sizeof(buf),fp_mbox); // read signature line
      strcpy(mbox[jC*NL+jL].code,&buf[1]);
      fprintf(stderr,"processing%s %s\n",buf,mbox[jC*NL+jL].code);
      assert(buf[0] == '#');
      for( int j = 0; j < NF; j++ )
      {
        fscanf(fp_mbox,"%lf %lf\n",&rp,&ip);
        mbox[jC*NL+jL].S11[j].freq = fa + (fb-fa)*(double)j/(double)(NF-1);
        mbox[jC*NL+jL].S11[j].spar = rp + I*ip;
      }
      
      for( int j = 0; j < NF; j++ )
      {
        fscanf(fp_mbox,"%lf %lf\n",&rp,&ip);
        mbox[jC*NL+jL].S21[j].freq = fa + (fb-fa)*(double)j/(double)(NF-1);
        mbox[jC*NL+jL].S21[j].spar = rp + I*ip;
      }

      for( int j = 0; j < NF; j++ )
      {
        fscanf(fp_mbox,"%lf %lf\n",&rp,&ip);
        mbox[jC*NL+jL].S12[j].freq = fa + (fb-fa)*(double)j/(double)(NF-1);
        mbox[jC*NL+jL].S12[j].spar = rp + I*ip;
      }

      for( int j = 0; j < NF; j++ )
      {
        fscanf(fp_mbox,"%lf %lf\n",&rp,&ip);
        mbox[jC*NL+jL].S22[j].freq = fa + (fb-fa)*(double)j/(double)(NF-1);
        mbox[jC*NL+jL].S22[j].spar = rp + I*ip;
      }
    }
  }
  fclose(fp_mbox);

  // // Now we are going to read in the antenna file, in a similar manner

  // allocating memory
  fs_pair* ant_s11 = (fs_pair*)malloc(NF_A*sizeof(fs_pair));
  
  // open the file, slurp the file.   Curly braces are just for grouping
  {
    char buf[128];
    FILE* fp_ant = fopen(argv[2],"r");
    assert( fp_ant );
    
    // eat header line(s)
    fgets(buf,sizeof(buf),fp_ant);
    //fgets(buf,sizeof(buf),fp_ant);
    //fgets(buf,sizeof(buf),fp_ant);
    //fgets(buf,sizeof(buf),fp_ant);

    for( int j = 0; j < NF_A; j++ )
    {
      double freq; double rp, ip;
      fscanf(fp_ant,"%lf %lf %lf\n",&freq,&rp,&ip);
      ant_s11[j].freq = freq/1e6;
      ant_s11[j].spar  = rp + I*ip;
    }
    fclose(fp_ant);
  }

  // Interactive loop
  while(1)
  {
    char buf[128];
    int freq_index;
    complex double S11_ant;    
    fprintf(stdout,"enter frequency in MHz:");
    (void)fgets(buf,sizeof(buf),stdin);
    double const freq = atof(buf);
    // find in antenna file ...
    interpolate(ant_s11,NF_A,freq,&S11_ant);   // output the interpolated antenna S11
    fprintf(stdout,"match for (%8.4f,%8.4f) at %8.4f MHz\n",RI(S11_ant),freq);

    // search for match ...
    int best_index = -1;
    int best_rev_bit = -1;
    double best_vswr = 1e6;
    int rev_bit;
    for( int j = 0; j < NL*NC; j++ )
    {
      rev_bit = 0;
      complex double S[2*2];
      interpolate(mbox[j].S11,NF,freq,&S[0]);  // interpolation...
      interpolate(mbox[j].S12,NF,freq,&S[1]);
      interpolate(mbox[j].S21,NF,freq,&S[2]);
      interpolate(mbox[j].S22,NF,freq,&S[3]);    

      // now finally calculate gamma and then from this VSWR.  
      // doing this in both directions (might need to swap connections remember)
      // Iterate through all VSWRs calculated and keep track of the best one so far
      // including both forward and reverse (connections swapped)
      complex double gamma;
      s22_to_s11_fwd(S,S11_ant,&gamma);
      double const cand_vswr_f = fabs((1.0 + mag(gamma))/(1.0 - mag(gamma)));
      s22_to_s11_bkw(S,S11_ant,&gamma);
      double const cand_vswr_r = fabs((1.0 + mag(gamma))/(1.0 - mag(gamma)));
      double cand_vswr = cand_vswr_f;    
      if( cand_vswr_r < cand_vswr_f )
      {
        rev_bit = 1;
        cand_vswr = cand_vswr_r;
      }
      if( cand_vswr < best_vswr )
      {
        best_vswr = cand_vswr;
        best_index = j;
        best_rev_bit = rev_bit;
      } 
    } 
    assert( best_index != -1 );
    assert( best_index >= 0 );
    assert( best_index < NC*NL );

    // preparing for output to user
    mbox[best_index].code[2] = '\0';
    int const index = atof(mbox[best_index].code);
    assert( index <= 11 );
    assert( index > 0 );
    const char r_tab[12] = "ABCDEFGHIJKL";

    if( best_rev_bit == 0 )
      fprintf(stdout,"vswr %f at %c %s\n",best_vswr,r_tab[index],&mbox[best_index].code[3]);
    else
      fprintf(stdout,"vswr %f at %c %s (swap connectors)\n",best_vswr,r_tab[index],&mbox[best_index].code[3]);
  }

  // right now we never reach this to free memory.   User quits with ^C and OS cleans up.
  // should add a user quit option.
  for( int jL = 0; jL < NL; jL++ )
    for( int jC = 0; jC < NC; jC++ )
    {
      free(mbox[jC*NL+jL].S11);
      free(mbox[jC*NL+jL].S21);
      free(mbox[jC*NL+jL].S12);
      free(mbox[jC*NL+jL].S22);      
    }
  
  free(mbox);
  free(ant_s11);  
}


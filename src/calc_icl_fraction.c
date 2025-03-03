// Print out the fraction of infalling satellite galaxies that are merged into 
// central halos, i.e., merged_frac, as a function of halo mass and redshift.
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "observations.h"
#include "smf.h"
#include "all_smf.h"
#include "distance.h"
#include "integrate.h"
#include "mlist.h"
#include "calc_sfh.h"
#include "mah.h"

#define NUM_ZS 9
extern int64_t num_outputs;
extern struct timestep *steps;

int main(int argc, char **argv)
{
  struct smf_fit the_smf;
  int i, j;

  if (argc < 3) 
  {
    fprintf(stderr, "Usage: %s mass_cache parameter_file (> output_file)\n", argv[0]);
    exit(1);
  }

  // Read in model parameters
  FILE *param_input = check_fopen(argv[2], "r");
  char buffer[2048];
  fgets(buffer, 2048, param_input);
  read_params(buffer, smf.params, NUM_PARAMS);

  // Turn off the built-in GSL error handler that kills the program
  // when an error occurs. We handle the errors manually.
  gsl_set_error_handler_off();
  // We use non-linear scaling relation between the radiative and total Eddington ratios.
  nonlinear_luminosity = 1;
  // Set up the PSF for stellar mass functions. See observations.c.
  setup_psf(1);
  // Load cached halo mass functions.
  load_mf_cache(argv[1]);
  // Initialize all the timesteps/snapshots.
  init_timesteps();
  // Calculate the star-formation histories and black hole histories. See calc_sfh.c.
  calc_sfh(&the_smf);
  
  // Print out the merged fraction.
  for (i=1; i<num_outputs-1; i++) 
  {
    printf("%f ", steps[i].scale);
    for (j=11; j<16; j++) 
    {
      int b = (j-M_MIN)*BPDEX;
      printf("%e ", steps[i].merged_frac[b]);
    }
    printf("\n");
  }
  return 0;
}

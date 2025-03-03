// Calculate the cosmic star formation rates and SMBH accretion rates
// at each snapshot.
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

int main(int argc, char **argv)
{
  float z;
  struct smf_fit the_smf;
  int i;
  
  if (argc < 3) 
  {
    fprintf(stderr, "Usage: %s mass_cache parameter_file (> output_file)\n", argv[0]);
    exit(1);
  }

  // Read in model parameters
  FILE *param_input = check_fopen(argv[2], "r");
  char buffer[2048];
  fgets(buffer, 2048, param_input);
  read_params(buffer, the_smf.params, NUM_PARAMS);
  
  // Fix some of the model parameters.
  assert_model(&the_smf);
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
  printf("#z csfr_obs csfr cbhar_obs cbhar\n");
  for (i=0; i < num_outputs; i++)
  {
    z = 1 / steps[i].scale - 1;
    printf("%f %e %e %e %e\n", z, steps[i].observed_cosmic_sfr, steps[i].cosmic_sfr, steps[i].observed_cosmic_bhar, steps[i].cosmic_bhar);
  }

  return 0;
}

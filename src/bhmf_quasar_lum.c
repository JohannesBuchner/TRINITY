// Calculate the black hole mass function within a certain bolometric luminosity
// threshold (lbol_low, lbol_high) at a certain redshift, z.
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
#include "expcache2.h"
#include "universe_time.h"

int main(int argc, char **argv)
{
  int64_t i, j, k, l;
  struct smf_fit smf;
  // Calculate the black hole mass function within a certain luminosity bin (lbol_low, lbol_high) at a certain redshift.
  if (argc < 6) 
  {
    fprintf(stderr, "Usage: %s mass_cache param_file z lbol_low(in erg/s) lbol_high(in erg/s) (> output_file)\n", argv[0]);
    exit(1);
  }
  // Read in model parameters, redshift, and bolometric luminosity intervals.
  FILE *param_input = check_fopen(argv[2], "r");
  char buffer[2048];
  fgets(buffer, 2048, param_input);
  read_params(buffer, smf.params, NUM_PARAMS);

  double z = atof(argv[3]);
  double Lbol_low = atof(argv[4]);
  double Lbol_high = atof(argv[5]);

  // Fix model parameters
  assert_model(&smf);
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
  INVALID(smf) = 0;

  // Calculate the star-formation histories and black hole histories. See calc_sfh.c.
  calc_sfh(&smf);
  printf("#Is the model invalid? %e\n", INVALID(smf));
  printf("#z=%.2f, Lbol_low=%.6f, Lbol_high=%.6f\n", z, Lbol_low, Lbol_high);
  printf("#Mbh Phi(Mbh)\n");

  // Find the snapshot that is the closest to the given redshift.
  double t,m;
  int64_t step;
  double f;
  calc_step_at_z(z, &step, &f);

  double prob_Mh[M_BINS] = {0};

  // calculate the total scatter in BH mass at fixed ***halo mass***,
  // which is a quadratic sum of the scatter around the median black
  // hole mass--bulge mass relation, and that of the median stellar
  // mass--halo mass relation, enhanced by the slope of the black
  // hole mass--bulge mass relation.
  double sm_scatter = steps[step].smhm.scatter * steps[step].smhm.bh_gamma;
  double scatter = sqrt(sm_scatter*sm_scatter 
                      + steps[step].smhm.bh_scatter*steps[step].smhm.bh_scatter);
  double mbh_min = steps[step].bh_mass_min, mbh_max = steps[step].bh_mass_max;
  double mbh_inv_bpdex = (mbh_max - mbh_min) / MBH_BINS;

  // Calculate the probability of SMBHs' having luminosities between (lbol_low, lbol_high)
  // by interpolating the luminosity distribution.
  for (i=0; i<MBH_BINS; i++)
  {
    double mbh = mbh_min + (i + 0.5) * mbh_inv_bpdex;
    
    double lbol_f_low = (Lbol_low - LBOL_MIN) * LBOL_BPDEX;
    double lbol_f_high = (Lbol_high - LBOL_MIN) * LBOL_BPDEX;
    int64_t lbol_b_low = lbol_f_low; lbol_f_low -= lbol_b_low;
    int64_t lbol_b_high = lbol_f_high; lbol_f_high -= lbol_b_high;
    if (lbol_b_low >= LBOL_BINS - 1) {lbol_b_low = LBOL_BINS - 2; lbol_f_low = 1;}
    if (lbol_b_high >= LBOL_BINS - 1) {lbol_b_high = LBOL_BINS - 2; lbol_f_high = 1;}
    double prob_lum = (1 - lbol_f_low) * steps[step].lum_dist_full[i*LBOL_BINS+lbol_b_low];
    double prob_tot = 0;
    for (j=0; j<LBOL_BINS; j++) prob_tot += steps[step].lum_dist_full[i*LBOL_BINS+j];
    for (j=lbol_b_low+1; j<lbol_b_high; j++) prob_lum += steps[step].lum_dist_full[i*LBOL_BINS+j];
    prob_lum += lbol_f_high * steps[step].lum_dist_full[i*LBOL_BINS+lbol_b_high];

    double dc = steps[step].smhm.bh_duty;
    double f_mass = exp((mbh - steps[step].smhm.dc_mbh) / steps[step].smhm.dc_mbh_w);
    f_mass = f_mass / (1 + f_mass);
    dc *= f_mass;
    if (dc < 1e-4) dc = 1e-4;

    prob_lum /= prob_tot;
    prob_lum *= dc;
    // The mass function of SMBHs with luminosities between (lbol_low, lbol_high)
    printf("%f %e\n", mbh, prob_lum * steps[step].bhmf[i]);
  }
  return 0;
}

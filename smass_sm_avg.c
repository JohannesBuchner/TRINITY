#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "all_smf.h"
#include "observations.h"
#include "calc_sfh.h"
#include "sm_limits.h"
#include "expcache2.h"

#define SM_START 8
#define SM_END 12.1
#define SM_MAX 16
#define SM_MIN 0
#define MASS_MIN 7.0
#define MASS_MAX 16.0
#define ONE_SIGMA 0.682689492137
#define SIGMA_UP ((int)(num_entries*(0.5+ONE_SIGMA/2.0)))
#define SIGMA_DOWN ((int)(num_entries*(0.5-ONE_SIGMA/2.0)))

#define MASS_BINS 1000
#define MASS_STEP (float)((MASS_MAX-MASS_MIN)/(MASS_BINS-1))

float sm_m[MASS_BINS];
float dens_cache[MASS_BINS];

void read_params(char *buffer, double *params, int max_n);
int float_compare(const void *a, const void *b);

void gen_dens_cache(float scale) {
  int i;
  float m;
  for (i=0; i<MASS_BINS; i++) {
    m = MASS_MIN + i*MASS_STEP;
    dens_cache[i] = exp10fc(mf_cache(scale, m));
  }
}

float avg_m(float sm, struct smf c) {
  int i;
  float m;
  float w=0, wm=0, dw;
  float inv_scatter2 = 0.5/(c.obs_scatter*c.obs_scatter + c.scatter*c.scatter);
  for (i=0; i<MASS_BINS; i++) {
    m = MASS_MIN + i*MASS_STEP;
    float dm = sm - sm_m[i];
    dw = exp(-dm*dm*inv_scatter2)*dens_cache[i];
    w+=dw;
    wm+=dw*m;
  }
  return (wm/w);
}

int main(int argc, char **argv) {
  // float z, sm, m, mass_start;
  // FILE *input;
  // char buffer[1024];
  // struct smf *fits;
  // struct smf_fit smf_fit;
  // float *smmrs;
  // float sm_min, sm_max;
  // int i, num_entries;

  // if (argc<5) {
  //   printf("Usage: %s z num_entries mf_cache red_smf_mcmc.dat\n", argv[0]);
  //   exit(1);
  // }
  
  // z = atof(argv[1]);
  // sm_min = sm_min_mass(z);
  // sm_max = sm_max_mass(z);
  // num_entries = atol(argv[2]);
  // fits = (struct smf *)malloc(sizeof(struct smf)*num_entries);
  // smmrs = (float *)malloc(sizeof(float)*num_entries);

  // load_mf_cache(argv[3]);

  // if (!(input = fopen(argv[4], "r"))) {
  //   printf("Couldn't open file %s for reading!\n", argv[5]);
  //   exit(1);
  // }

  // for (i=0; i<num_entries; i++) {
  //   if (!fgets(buffer, 1024, input)) break;
  //   read_params(buffer, smf_fit.params, NUM_PARAMS);
  //   fits[i] = smhm_at_z(z, smf_fit);
  //   if (!i) { fits[i].sm_0 += MU(smf_fit); }
  // }
  // fclose(input);
  // num_entries = i;

  // mass_start = MASS_MIN;
  // //mass_end = MASS_MAX;
  // //if (mass_end > 15.25 - z*0.5) mass_end = 15.25 - z*0.5;

  // for (i=0; i<MASS_BINS; i++) {
  //   m = MASS_MIN + i*MASS_STEP;
  //   sm_m[i] = calc_sm_at_m(m, fits[0]);
  // }
  // gen_dens_cache(1.0/(z+1.0));
  // printf("#Log10(Halo Mass) [Msun] Log10(SM/HM) Log10(SM) [Msun]\n");
  // for (sm = SM_START; sm<SM_END; sm+=0.1) {
  //   if (sm < sm_min) continue;
  //   if (sm > sm_max) continue;
  //   m = avg_m(sm, fits[0]);
  //   printf("%f %f %f\n", m, sm-m, sm);
  // }
  return 0;
}

int float_compare(const void *a, const void *b) {
  float c = *((float *)a);
  float d = *((float *)b);
  if (c<d) return -1;
  if (c>d) return 1;
  return 0;
}


void read_params(char *buffer, double *params, int max_n) {
  int num_entries = 0;
  char *cur_pos = buffer, *end_pos;
  float val = strtod(cur_pos, &end_pos);
  while (cur_pos != end_pos && num_entries < max_n) {
    params[num_entries] = val;
    num_entries++;
    cur_pos=end_pos;
    while (*cur_pos==' ' || *cur_pos=='\t' || *cur_pos=='\n') cur_pos++;
    val = strtod(cur_pos, &end_pos);
  }
}

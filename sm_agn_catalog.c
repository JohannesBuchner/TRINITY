#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "observations.h"
#include "smf.h"
#include "all_smf.h"
#include "distance.h"
#include "integrate.h"
#include "mlist.h"
#include "calc_sfh.h"
#include "mah.h"
#include "check_syscalls.h"
#include "expcache2.h"
#include "stringparse.h"
#include "mt_rand.h"
//#include "inthash.h"

extern int64_t num_outputs;
extern struct timestep *steps;

struct shm {
  float sm, hm;
};

struct shm *halos = NULL;

float biterp (float a, float b, float c, float d, float f1, float f2) {
  float al = log10(a);
  float bl = log10(b);
  float cl = log10(c);
  float dl = log10(d);
  float e = al+f1*(bl-al);
  float f = cl+f1*(dl-cl);
  return (e+f2*(f-e));
}


void add_sm_to_catalog(struct smf_fit f, char *filename) {
  char buffer[1024];
  FILE *input, *output;
  int n, ts, i;
  float sm, obs_sm;
  float mbh; 
  double p_eta[4] = {0};
  double p_lbol[5] = {0};
  double eta_aird = -1; //The selection criteria from Aird2019.
  double lbol_aird = 45;
  double p_aird = 0;
#define NUM_CAT_INPUTS 53
  int64_t id, pid, desc_id, num_prog, upid, desc_pid, phantom, mmp, bfid, dfid, trid, ohid, sn, ncdfid, lpdfid;
  float scale, desc_scale, sam_mvir, mvir, rvir, rs, vrms, scale_of_last_MM, vmax, x,y,z, vx,vy,vz, Jx, Jy, Jz, spin, rs_klypin, mvir_all, m200b, m200c, m500c, m2500c, xoff, voff, spin_bullock, b_to_a, c_to_a, ax, ay, az, tu, macc, mpeak, vacc, vpeak;
  //struct inthash *ih = NULL;
  SHORT_PARSETYPE;
  enum parsetype t[NUM_CAT_INPUTS];
  enum short_parsetype st[NUM_CAT_INPUTS] = 
    {F,D64,F,D64,D64,
     D64,D64,D64,D64,F,
     F,F,F,F,D64,
     F,F,F,F,F,
     F,F,F,F,F,
     F,F,D64,D64,D64,
     D64,D64,D64,D64,F,
     F,F,F,F,F,
     F,F,F,F,F,
     F,F,F,F,F,
     F,F,F};

  void *data[NUM_CAT_INPUTS] = {&scale, &id, &desc_scale, &desc_id, &num_prog, 
                        &pid, &upid, &desc_pid, &phantom, &sam_mvir, &mvir, &rvir, 
                        &rs, &vrms, &mmp, &scale_of_last_MM, &vmax, &x, &y, &z, 
                        &vx, &vy, &vz, &Jx, &Jy, &Jz, &spin, 
                        &bfid, &dfid, &trid, &ohid, &sn, &ncdfid, &lpdfid, 
                        &rs_klypin, &mvir_all, &m200b, &m200c, &m500c, &m2500c, 
                        &xoff, &voff, &spin_bullock, &b_to_a, &c_to_a, 
                        &ax, &ay, &az, &tu, &macc, &mpeak, &vacc, &vpeak};

  //ih = new_inthash();
  
  strncpy(buffer, "Catalogs/uvlf_AGN_final_29_const_erdf/", 1024);
  //strncpy(buffer, filename, 1024);
  n = strlen(buffer);
  int n_filename = strlen(filename);
  //strncpy(&(buffer[n]), filename[n_filename-21], 21);
  strncpy(&(buffer[n]), filename + n_filename-18, 18);
  //strncpy(buffer, filename, 1024);
  //n = strlen(buffer);
  //strncpy(&(buffer[n-4]), "sm_catalog.list", 1024-n);
  input = check_fopen(filename, "r");
  fprintf(stderr, "filename=%s\n", filename);
  output = check_fopen(buffer, "w");
  fgets(buffer, 1024, input);  //Header line
  fprintf(output, "#scale(0) id(1) desc_scale(2) desc_id(3) num_prog(4) pid(5) upid(6) desc_pid(7) phantom(8) sam_mvir(9) mvir(10) rvir(11) rs(12) vrms(13) mmp?(14) scale_of_last_MM(15) vmax(16) x(17) y(18) z(19) vx(20) vy(21) vz(22) Jx(23) Jy(24) Jz(25) Spin(26) Breadth_first_ID(27) Depth_first_ID(28) Tree_root_ID(29) Orig_halo_ID(30) Snap_num(31) Next_coprogenitor_depthfirst_ID(32) Last_progenitor_depthfirst_ID(33) Rs_Klypin Mvir_all M200b M200c M500c M2500c Xoff Voff Spin_Bullock b_to_a c_to_a A[x] A[y] A[z] T/|U| Macc Mpeak Vacc Vpeak Log(SM) Log(SM_obs) Log(SFR_obs) Mbh p_eta_0001 p_eta_001 p_eta_01 p_eta_1 p_lbol_41 p_lbol_42 p_lbol_43 p_lbol_44 p_lbol_45 p_eta01_lbol45_Aird\n");

  ts = num_outputs-2;
  while (1) {
    fgets(buffer, 1024, input);  //Header line
    if (buffer[0] != '#') break;
    fprintf(output, "%s", buffer);
  }
  fprintf(output, "#Log(SM): Log10 of true stellar mass (in Msun)\n");
  fprintf(output, "#Log(SM_obs): Log10 of observed stellar mass (in Msun)\n");
  fprintf(output, "#Log(SFR_obs): Log10 of observed star formation rate (in Msun/yr)\n");

  for (n=0; n<NUM_CAT_INPUTS; n++) t[n] = st[n];

  int64_t num_parents = 0;
  do {
    n = stringparse(buffer, data, t, NUM_CAT_INPUTS);
    if (n < NUM_CAT_INPUTS) continue;
    while (steps[ts].scale > scale && ts>0) ts--;
    if (!ts) break;

    float tf = (scale - steps[ts].scale) / (steps[ts+1].scale - steps[ts].scale);
    if (!(mpeak>0) || !(mvir>0)) continue;
    float logm = log10(mpeak/0.678);
    // float sm1 = calc_sm_at_m(logm, steps[ts].smhm);
    // float sm2 = calc_sm_at_m(logm, steps[ts+1].smhm);
    float sm1 = calc_sm_at_m(logm, steps[ts]);
    float sm2 = calc_sm_at_m(logm, steps[ts+1]);
    sm = sm1 + tf*(sm2-sm1);
#define INTERP(y) (steps[ts].smhm.y + tf*(steps[ts+1].smhm.y - steps[ts].smhm.y))
    float ins_scatter = INTERP(scatter);
    float obs_scatter = INTERP(obs_scatter);
    float rho = INTERP(sfr_sm_corr);
    float mu = INTERP(mu);
    float kappa = INTERP(kappa);

    float bh_gamma = INTERP(bh_gamma);
    float bh_beta = INTERP(bh_beta);

    float bh_scatter = INTERP(bh_scatter);
    float bh_duty = INTERP(bh_duty);
    float dc_mbh = INTERP(dc_mbh);
    float dc_mbh_w = INTERP(dc_mbh_w);

    float sm_offset = normal_random(0, 1);
    float sfr_offset = normal_random(0, 1);
    float mbh_offset = normal_random(0, 1);
    sm = sm + sm_offset*ins_scatter;
    obs_sm = sm + normal_random(0, obs_scatter) + mu;

    float bm = bulge_mass(obs_sm, scale);
    mbh = bh_beta + bh_gamma * (bm - 11.0);
    mbh += mbh_offset * bh_scatter;


    double f_mass = exp((mbh - dc_mbh) / dc_mbh_w);
    f_mass = f_mass / (1 + f_mass);
    // f_mass = f_mass < 1? f_mass : 1;
    bh_duty *= f_mass;
    if (bh_duty < 1e-4) bh_duty = 1e-4;

    //fprintf(stderr, "mh=%f, sm=%f, obs_sm=%f, mbh=%f\n", logm, sm, obs_sm, mbh);  

    
    float mf = (logm - M_MIN)*BPDEX;
    int64_t j = mf;
    mf -= j;
    float sfr = biterp(steps[ts].sfr[j], steps[ts+1].sfr[j],
			 steps[ts].sfr[j+1], steps[ts+1].sfr[j+1],
			 tf, mf);
    float scatter_corr = log10(exp(pow(ins_scatter*log(10), 2)/2.0));
    sfr -= scatter_corr;
    sfr += ins_scatter*(sm_offset*rho + sfr_offset*sqrt(1.0-rho*rho));
    if (pid >= 0) { sfr += log10(mvir/mpeak); }
    // if (sfr - sm > 11) obs_sm += kappa;
    sfr += mu + kappa;


    // Calculate the cumulative probabilities of hosting AGN above certain Eddington ratio
    // or luminosity thresholds.
    for (int k=0; k<5; k++)
    {
      // Eddington ratio thresholds. Only calculate when k < 4, because we only have
      // 4 threshold values.
      
      float eta_crit;
      double prob1, prob2, bher_f;
      int64_t bher_b;

      float mf = (logm - M_MIN) * BPDEX;
      int64_t mb = mf;
      mf -= mb;
      if (mb == M_BINS - 1) {mb = M_BINS - 2; mf = 1;}


      if (k < 4)
      {

        // float eta_crit = -3.0 + k;
        eta_crit = -3.0 + k;

        // float mf = (logm - M_MIN) * BPDEX;
        // int64_t mb = mf;
        // mf -= mb;
        // if (mb == M_BINS - 1) {mb = M_BINS - 2; mf = 1;}

        // for the mb-th halo mass bin
        // double prob1 = 0.0;
        prob1 = 0.0;
        // double bher_f = (eta_crit - steps[ts].bh_eta[mb] - steps[ts].ledd_min[mb]) * steps[ts].ledd_bpdex[mb];
        // int64_t bher_b = bher_f;
        bher_f = (eta_crit - steps[ts].bh_eta[mb] - steps[ts].ledd_min[mb]) * steps[ts].ledd_bpdex[mb];
        bher_b = bher_f;
        bher_f -= bher_b;
        if (bher_b >= BHER_BINS - 1) {bher_b = BHER_BINS - 2; bher_f = 1;}

        prob1 += (1 - bher_f) * steps[ts].bher_dist[mb*BHER_BINS+bher_b];
        for (int ii=bher_b+1; ii<BHER_BINS; ii++)
          prob1 += steps[ts].bher_dist[mb*BHER_BINS+ii];
        if (steps[ts].bher_dist_norm[mb] > 0) prob1 /= steps[ts].bher_dist_norm[mb];

        // for the (mb+1)-th halo mass bin
        // double prob2 = 0.0;
        prob2 = 0.0;
        bher_f = (eta_crit - steps[ts].bh_eta[mb+1] - steps[ts].ledd_min[mb+1]) * steps[ts].ledd_bpdex[mb+1];
        bher_b = bher_f;
        bher_f -= bher_b;
        if (bher_b >= BHER_BINS - 1) {bher_b = BHER_BINS - 2; bher_f = 1;}

        prob2 += (1 - bher_f) * steps[ts].bher_dist[(mb+1)*BHER_BINS+bher_b];
        for (int ii=bher_b+1; ii<BHER_BINS; ii++)
          prob2 += steps[ts].bher_dist[(mb+1)*BHER_BINS+ii];
        if (steps[ts].bher_dist_norm[(mb+1)] > 0) prob2 /= steps[ts].bher_dist_norm[(mb+1)];

        // interpolate between the mb-th and the (mb+1)-th mass bin.
        p_eta[k] = (prob1 + mf * (prob2 - prob1)) * bh_duty;
      }
      

      //fprintf(stderr, "mbh=%f, eta_crit=%f, p_eta[%d]=%e, prob1=%e, prob2=%e, bher_b=%d, bher_f=%f, bher_dist_norm=%e\n", mbh, eta_crit, k, p_eta[k], prob1, prob2, bher_b, bher_f, steps[ts].bher_dist_norm[mb+1]); 

      // luminosity thresholds.
      float lbol_crit = 41.0 + k;
      eta_crit = lbol_crit - 38.1 - mbh;


      // for the mb-th halo mass bin
      prob1 = 0.0;
      bher_f = (eta_crit - steps[ts].bh_eta[mb] - steps[ts].ledd_min[mb]) * steps[ts].ledd_bpdex[mb];
      bher_b = bher_f;
      bher_f -= bher_b;
      if (bher_b >= BHER_BINS - 1) {bher_b = BHER_BINS - 2; bher_f = 1;}

      prob1 += (1 - bher_f) * steps[ts].bher_dist[mb*BHER_BINS+bher_b];
      for (int ii=bher_b+1; ii<BHER_BINS; ii++)
        prob1 += steps[ts].bher_dist[mb*BHER_BINS+ii];
      if (steps[ts].bher_dist_norm[mb] > 0) prob1 /= steps[ts].bher_dist_norm[mb];

      // for the (mb+1)-th halo mass bin
      prob2 = 0.0;
      bher_f = (eta_crit - steps[ts].bh_eta[mb+1] - steps[ts].ledd_min[mb+1]) * steps[ts].ledd_bpdex[mb+1];
      bher_b = bher_f;
      bher_f -= bher_b;
      if (bher_b >= BHER_BINS - 1) {bher_b = BHER_BINS - 2; bher_f = 1;}

      prob2 += (1 - bher_f) * steps[ts].bher_dist[(mb+1)*BHER_BINS+bher_b];
      for (int ii=bher_b+1; ii<BHER_BINS; ii++)
        prob2 += steps[ts].bher_dist[(mb+1)*BHER_BINS+ii];
      if (steps[ts].bher_dist_norm[(mb+1)] > 0) prob2 /= steps[ts].bher_dist_norm[(mb+1)];

      // interpolate between the mb-th and the (mb+1)-th mass bin.
      p_lbol[k] = (prob1 + mf * (prob2 - prob1)) * bh_duty;
      //fprintf(stderr, "mbh=%f, lbol_crit=%f, p_lbol[%d]=%e\n", mbh, lbol_crit, k, p_lbol[k]);
    }

    float eta_lbol_Aird = lbol_aird - 38.1 - mbh;
    // If the Eddington ratio that gives logLbol = 45 is bigger than 0.1, then we 
    // take the prob value for logLbol = 45.
    p_aird = eta_lbol_Aird > eta_aird ? p_lbol[4] : p_eta[2];

    int64_t uber_pid = (upid >= 0) ? upid : id;
    //int64_t offset = ih_getint64(ih, uber_pid);
    //if (offset == IH_INVALID) {
    //  if (!(num_parents%1000))
	//halos = check_realloc(halos, sizeof(struct shm)*(num_parents+1000), "AH");
      //offset = num_parents;
      //halos[offset].sm = 0;
      //halos[offset].hm = mvir;
      //num_parents++;
      //ih_setint64(ih, uber_pid, offset);
    //}
    //if (mvir > halos[offset].hm) halos[offset].hm = mvir;
    //halos[offset].sm += pow(10, sm)*0.7;

    fprintf(output, "%.4f %8"PRId64" %.4f %8"PRId64" %6"PRId64" %8"PRId64" %8"PRId64" %8"PRId64" %2"PRId64" %.5e %.5e %6f %6f %6f %2"PRId64" %.4f %6f %.5f %.5f %.5f %.3f %.3f %.3f %.3e %.3e %.3e %.5f %"PRId64" %"PRId64" %"PRId64" %"PRId64" %"PRId64" %"PRId64" %"PRId64" %.5f %.4e %.4e %.4e %.4e %.4e %.5f %.2f %.5f %.5f %.5f %.5f %.5f %.5f %.4f %.5e %.5e %6f %6f %f %f %f %f %e %e %e %e %e %e %e %e %e %e\n",
	    scale, id, desc_scale, desc_id, num_prog, pid, upid, desc_pid, phantom, sam_mvir, mvir, rvir, rs, vrms, mmp, scale_of_last_MM, vmax, x, y, z, vx, vy, vz, Jx, Jy, Jz, spin, bfid, dfid, trid, ohid, sn, ncdfid, lpdfid, rs_klypin, mvir_all, m200b, m200c, m500c, m2500c, xoff, voff, spin_bullock, b_to_a, c_to_a, ax, ay, az, tu, macc, mpeak, vacc, vpeak, sm, obs_sm, sfr, mbh, p_eta[0], p_eta[1], p_eta[2], p_eta[3], p_lbol[0], p_lbol[1], p_lbol[2], p_lbol[3], p_lbol[4], p_aird);
  } while (fgets(buffer, 1024, input));
  fclose(input);
  fclose(output);
  //free_inthash(ih);

  //strncpy(buffer, "Catalogs/uvlf_AGN_final_29_const_erdf/", 1024);
  //strncpy(buffer, filename, 1024);
  //n = strlen(buffer);
  //int n_filename = strlen(filename);
  //strncpy(&(buffer[n]), filename[n_filename-21], 1024-n);
  //output = check_fopen(buffer, "w");
  //for (i=0; i<num_parents; i++) {
  //  fprintf(output, "%e %f\n", halos[i].hm, (halos[i].sm/halos[i].hm)/0.16);
  //}
  //fclose(output);
}


int main(int argc, char **argv)
{
  int i;
  enum parsetype t[NUM_PARAMS];
  void *data[NUM_PARAMS];
  char buffer[1024] = {0};
  struct smf_fit base_smf;

  if (argc<3) {
    fprintf(stderr, "Usage: %s mass_cache catalog1 ... < mcmc_output\n", argv[0]);
    exit(1);
  }
  for (i=0; i<NUM_PARAMS; i++) {
    t[i] = PARSE_FLOAT64;
    data[i] = &(base_smf.params[i]);
  }

  fgets(buffer, 1024, stdin);
  if (stringparse(buffer, data, t, NUM_PARAMS) != NUM_PARAMS) {
    fprintf(stderr, "%d\n", stringparse(buffer, data, t, NUM_PARAMS));
    fprintf(stderr, "Could not parse input!\n");
    exit(1);
  }

  r250_init(87L);
  gen_exp10cache();
  setup_psf(1);
  gsl_set_error_handler_off();
  load_mf_cache(argv[1]);
  init_timesteps();
  INVALID(base_smf) = 0;
  calc_sfh(&base_smf);

#pragma omp for schedule(guided,5)
  for (i=2; i<argc; i++)
    add_sm_to_catalog(base_smf, argv[i]);
  return 0;
}

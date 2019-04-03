#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the dab-cmdline
 *    dab-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dab-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab-cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"tii_detector.h"

#include <utility>
#include <numeric>

#include	<stdio.h>
#include	<inttypes.h>

// configure / adapt these
#define MIN_SNR_POWER_RATIO		3.0F	/* 3.0 == 4.7 dB */
#define MAX_NUM_TII			7	/* evaluate up to 7 TII-IDs */
#define NUM_MINS_FOR_NOISE		4	/* use lowest 4 carrier pairs to determine noise level */

#define OUT_STAT			0
#define LOG_WIN_SUM			0

// NOT for configuration - just avoid magic numbers in code
#define NUM_GROUPS				8
#define NUM_SUBIDS				24
#define NUM_TII_EVAL_CARRIERS	(NUM_GROUPS * NUM_SUBIDS * 2)		/* = 384 */
#define NUM_SUBIDS_IN_PATTERN	4	/* 8 groups in total. but only 4 active */
#define MIN_SNR				1E-20F	/* -200 dB */
#define MAX_SNR				1E20F	/* +200 dB */

static
uint8_t table [] = {
	// octal	binary					index
	0017,		// 0 0 0 0 1 1 1 1		0
	0027,		// 0 0 0 1 0 1 1 1		1
	0033,		// 0 0 0 1 1 0 1 1		2
	0035,		// 0 0 0 1 1 1 0 1		3
	0036,		// 0 0 0 1 1 1 1 0		4
	0047,		// 0 0 1 0 0 1 1 1		5
	0053,		// 0 0 1 0 1 0 1 1		6
	0055,		// 0 0 1 0 1 1 0 1		7
	0056,		// 0 0 1 0 1 1 1 0		8
	0063,		// 0 0 1 1 0 0 1 1		9

	0065,		// 0 0 1 1 0 1 0 1		10
	0066,		// 0 0 1 1 0 1 1 0		11
	0071,		// 0 0 1 1 1 0 0 1		12
	0072,		// 0 0 1 1 1 0 1 0		13
	0074,		// 0 0 1 1 1 1 0 0		14
	0107,		// 0 1 0 0 0 1 1 1		15
	0113,		// 0 1 0 0 1 0 1 1		16
	0115,		// 0 1 0 0 1 1 0 1		17
	0116,		// 0 1 0 0 1 1 1 0		18
	0123,		// 0 1 0 1 0 0 1 1		19

	0125,		// 0 1 0 1 0 1 0 1		20
	0126,		// 0 1 0 1 0 1 1 0		21
	0131,		// 0 1 0 1 1 0 0 1		22
	0132,		// 0 1 0 1 1 0 1 0		23
	0134,		// 0 1 0 1 1 1 0 0		24
	0143,		// 0 1 1 0 0 0 1 1		25
	0145,		// 0 1 1 0 0 1 0 1		26
	0146,		// 0 1 1 0 0 1 1 0		27
	0151,		// 0 1 1 0 1 0 0 1		28	
	0152,		// 0 1 1 0 1 0 1 0		29

	0154,		// 0 1 1 0 1 1 0 0		30
	0161,		// 0 1 1 1 0 0 0 1		31
	0162,		// 0 1 1 1 0 0 1 0		32
	0164,		// 0 1 1 1 0 1 0 0		33
	0170,		// 0 1 1 1 1 0 0 0		34
	0207,		// 1 0 0 0 0 1 1 1		35
	0213,		// 1 0 0 0 1 0 1 1		36
	0215,		// 1 0 0 0 1 1 0 1		37
	0216,		// 1 0 0 0 1 1 1 0		38
	0223,		// 1 0 0 1 0 0 1 1		39

	0225,		// 1 0 0 1 0 1 0 1		40
	0226,		// 1 0 0 1 0 1 1 0		41
	0231,		// 1 0 0 1 1 0 0 1		42
	0232,		// 1 0 0 1 1 0 1 0		43
	0234,		// 1 0 0 1 1 1 0 0		44
	0243,		// 1 0 1 0 0 0 1 1		45
	0245,		// 1 0 1 0 0 1 0 1		46
	0246,		// 1 0 1 0 0 1 1 0		47
	0251,		// 1 0 1 0 1 0 0 1		48
	0252,		// 1 0 1 0 1 0 1 0		49

	0254,		// 1 0 1 0 1 1 0 0		50
	0261,		// 1 0 1 1 0 0 0 1		51
	0262,		// 1 0 1 1 0 0 1 0		52
	0264,		// 1 0 1 1 0 1 0 0		53
	0270,		// 1 0 1 1 1 0 0 0		54
	0303,		// 1 1 0 0 0 0 1 1		55
	0305,		// 1 1 0 0 0 1 0 1		56
	0306,		// 1 1 0 0 0 1 1 0		57
	0311,		// 1 1 0 0 1 0 0 1		58
	0312,		// 1 1 0 0 1 0 1 0		59

	0314,		// 1 1 0 0 1 1 0 0		60
	0321,		// 1 1 0 1 0 0 0 1		61
	0322,		// 1 1 0 1 0 0 1 0		62
	0324,		// 1 1 0 1 0 1 0 0		63
	0330,		// 1 1 0 1 1 0 0 0		64
	0341,		// 1 1 1 0 0 0 0 1		65
	0342,		// 1 1 1 0 0 0 1 0		66
	0344,		// 1 1 1 0 0 1 0 0		67
	0350,		// 1 1 1 0 1 0 0 0		68
	0360		// 1 1 1 1 0 0 0 0		69
};

		TII_Detector::TII_Detector (uint8_t dabMode):
	                                          params (dabMode),
	                                          my_fftHandler (dabMode) {
int16_t	i;
double	winSum = 0.0;
double	winElem;

	this	-> T_u		= params. get_T_u ();
	carriers		= params. get_carriers ();
	theBuffer. resize	(T_u);

	fillCount		= 0;
	fft_buffer		= my_fftHandler. getVector ();	
	isFirstAdd	= true;
	numUsedBuffers	= 0;

	window. resize 		(T_u);

	for (i = 0; i < T_u; i ++) {
	   winElem  = (0.42 -
	               0.5 * cos (2 * M_PI * (float)i / T_u) +
	               0.08 * cos (4 * M_PI * (float)i / T_u));
	   winSum += winElem;
	}

	for (i = 0; i < T_u; i ++) {
	   winElem  = (0.42 -
	               0.5 * cos (2 * M_PI * (float)i / T_u) +
	               0.08 * cos (4 * M_PI * (float)i / T_u));
	   window [i] = winElem / winSum;
	}

#if LOG_WIN_SUM
	fprintf(stderr, "TII_Detector: T_u %d, window sum was: %f\n", T_u, winSum);
#endif

	for (i = 0; i < 256; ++i)
	    invTable [i] = 100;		// initialize with an invalid mainId!
	for (i = 0; i < 70; ++i)
	    invTable [table [i]] = i;
}

		TII_Detector::~TII_Detector (void) {
}

static
uint8_t bits [] = {128, 64, 32, 16, 8, 4, 2, 1};

//
void	TII_Detector::reset (void) {
	isFirstAdd	= true;
	numUsedBuffers	= 0;
}

//	To eliminate (reduce?) noise in the input signal, we might
//	add a few spectra before computing (up to the user)
void	TII_Detector::addBuffer (std::vector<std::complex<float>> v,
	                         float alfa, int32_t cifCounter) {
int	i;

//	apply reset()
	if (isFirstAdd) {
	   for (i = 0; i < T_u; i ++)
	      theBuffer [i] = std::complex<float> (0, 0);
	   memset (&P_allAvg [0], 0, T_u * sizeof (float));
	}

#if OUT_STAT
	float timeMaxNorm = std::norm (v[0]);
	for (int j = 1; j < T_u; j ++) {
	   float n = std::norm (v[j]);
	   if (n > timeMaxNorm)
	      timeMaxNorm = n;
	}
#endif

//	windowing + FFT
	for (i = 0; i < T_u; i ++)
//	   fft_buffer [i] = cmul (v [i], window [i]);
	   fft_buffer [i] = v [i];
	my_fftHandler. do_FFT ();

	for (int j = 0; j < T_u; ++j ) {
	   P_tmpNorm [j] = std::norm (fft_buffer [j]);
	}

#if OUT_STAT
	float specMaxNorm = std::norm( P_tmpNorm[0] );
	for (int j = 1; j < T_u; ++j ) {
	   if (P_tmpNorm [j] > specMaxNorm)
	      specMaxNorm = P_tmpNorm [j];
	}

	float timeMag = sqrt (timeMaxNorm);
	float specMag = sqrt (specMaxNorm);
	fprintf (stderr,
	         "TII_Detector addBuffer(): %d / %u: timeMax = %f  specMax = %f\n",
	         cifCounter, numUsedBuffers, timeMag, specMag);
	
	if (specMaxNorm > 32768.0F) {	// must be an error!  ( 128*128 *2 )
	   fprintf (stderr,
	            "TII_Detector addBuffer(): OVERFLOW! %d / %u: max norm() = %f\n",
	            cifCounter, numUsedBuffers, specMaxNorm);
	   return;		// do NOT use this spectrum
	}
#endif

	if (alfa < 0.0F || isFirstAdd) {
	   for (int j = 0; j < T_u; j++) {
	      P_allAvg  [j] += P_tmpNorm [j];
	      theBuffer [j] += fft_buffer [j];
	   }
	   isFirstAdd = false;
	   numUsedBuffers ++;
	}
	else {
	   const float alpha = alfa;
	   const float beta = 1.0F - alpha;
	   for (int j = 0; j < T_u; j++) {
	      P_allAvg [j] = alpha * P_allAvg [j] + beta * P_tmpNorm [j];
	      theBuffer [j] = alpha * theBuffer [j] +
	                      beta * fft_buffer [j];
	   }
	   numUsedBuffers ++;
	}
}

void	TII_Detector::collapse (std::complex<float> *inVec, float *outVec) {
int	i;

	for (i = 0; i < carriers / 8; i ++) {
	   int carr = - carriers / 2 + 2 * i;
	   outVec [i] = abs (real (inVec [(T_u + carr) % T_u] *
	                            conj (inVec [(T_u + carr + 1) % T_u])));

	   carr	= - carriers / 2 + 1 * carriers / 4 + 2 * i;
	   outVec [i] += abs (real (inVec [(T_u + carr) % T_u] *
	                            conj (inVec [(T_u + carr + 1) % T_u])));

	   carr	= - carriers / 2 + 2 * carriers / 4 + 2 * i + 1;
	   outVec [i] += abs (real (inVec [(T_u + carr) % T_u] *
	                            conj (inVec [(T_u + carr + 1) % T_u])));

	   carr	= - carriers / 2 + 3 * carriers / 4 + 2 * i + 1;
	   outVec [i] += abs (real (inVec [(T_u + carr) % T_u] *
	                            conj (inVec [(T_u + carr + 1) % T_u])));
	}
}


#define	NUM_GROUPS	8
#define	GROUPSIZE	24
void	TII_Detector::processNULL (int16_t *mainId, int16_t *subId) {
int i, j;
float	hulpTable	[NUM_GROUPS * GROUPSIZE];
float	C_table		[GROUPSIZE];	// contains the values
int	D_table		[GROUPSIZE];	// count of indices in C_table with data
float	avgTable	[NUM_GROUPS];
float	minTable	[NUM_GROUPS];
//
//	defaults:
	*mainId	= -1;
	*subId	= -1;

//	we map the "carriers" carriers (complex values) onto
//	a collapsed vector of "carriers / 8" length, 
//	considered to consist of 8 segments of 24 values
//	Each "value" is the sum of 4 pairs of subsequent carriers,
//	taken from the 4 quadrants -768 .. 385, 384 .. -1, 1 .. 384, 385 .. 768

	collapse (theBuffer. data (), hulpTable);
//
//	since the "energy levels" in the different GROUPSIZE'd values
//	may differ, we compute an average for each of the
//	NUM_GROUPS GROUPSIZE-value groups. 

	memset (avgTable, 0, NUM_GROUPS * sizeof (float));
	for (i = 0; i < NUM_GROUPS; i ++) {
	   minTable [i] = hulpTable [i * GROUPSIZE + 0];
	   avgTable [i] = 0;
	   for (j = 0; j < GROUPSIZE; j ++) {
	      avgTable [i] += hulpTable [i * GROUPSIZE + j];
	      if (hulpTable [i * GROUPSIZE + j] < minTable [i])
	         minTable [i] = hulpTable [i * GROUPSIZE + j];
	   }
	   avgTable [i] /= GROUPSIZE;
	}
//
//	Determining the offset is then easy, look at the corresponding
//	elements in the NUM_GROUPS sections and mark the highest ones
//	The summation of the high values are stored in the C_table,
//	the number of times the limit is reached in the group
//	is recorded in the D_table
//
//	Threshold 4 * avgTable is 6dB, we consider that a minimum
//	measurement shows that that is a reasonable value,
//	alternatively, we could take the "minValue" as reference
//	and "raise" the threshold. However, while that might be
//	too  much for 8-bit incoming values
	memset (D_table, 0, GROUPSIZE * sizeof (int));
	memset (C_table, 0, GROUPSIZE * sizeof (float));
//
	for (j = 0; j < NUM_GROUPS; j ++) {
	   for (i = 0; i < GROUPSIZE; i ++) {
	      if (hulpTable [j * GROUPSIZE + i] > 4 * avgTable [j]) {
//	         fprintf (stderr, "index (%d, %d) -> %f (%f)\n",
//	                           i, j,
//	             10 * log10 (hulpTable [j * GROUPSIZE + i] / avgTable [j]),
//	             10 * log10 (hulpTable [j * GROUPSIZE + i] / minTable [j]));
	         C_table [i] += hulpTable [j * GROUPSIZE + i];
	         D_table [i] ++;
	      }
	   }
	}

//
//	we extract from this result the two highest values that
//	meet the constraint of 4 values being sufficiently high
	float	Max_1	= 0;
	int	ind1	= -1;
	float	Max_2	= 0;
	int	ind2	= -1;

	for (j = 0; j < GROUPSIZE; j ++) {
	   if ((D_table [j] >= 4) && (C_table [j] > Max_1)) {
	      Max_2	= Max_1;
	      ind1	= ind2;
	      Max_1	= C_table [j];
	      ind1	= j;
	   }
	   else
	   if ((D_table [j] >= 4) && (C_table [j] > Max_2)) {
	      Max_2	= C_table [j];
	      ind2	= j;
	   }
	}
//
//	The - almost - final step is then to figure out which
//	groups contributed, obviously only where ind1 > 0
//	we start with collecting the values of the correct
//	elements of the NUM_GROUPS groups
//
//	for the qt-dab, we only need the "top" performer
	if (ind1 > 0) {
	   uint16_t pattern	= 0;
	   float x [NUM_GROUPS];
	   for (i = 0; i < NUM_GROUPS; i ++) 
	      x [i] = hulpTable [ind1 + GROUPSIZE * i];
//
//	we extract the four max values (it is known that they exist)
	   for (i = 0; i < 4; i ++) {
	      float	mmax	= 0;
	      int ind	= -1;
	      for (j = 0; j < NUM_GROUPS; j ++) {
	         if (x [j] > mmax) {
	            mmax = x [j];
	            ind  = j;
	         }
	      }
	      if (ind != -1) {
	         int index = ind;
	         x [ind] = 0;
	         pattern |= bits [ind];
	      }
	   }
//
//	The final step is then to do the mapping of the bits
//	The mainId is found using the match with the invTable
	   *mainId	= int (invTable [pattern]);
	   *subId	= ind1;
	   return;
	}
}


static inline float powerRatio( float s, float n ) {
	if ( s > n ) {
	    if ( s > n * 1E20F )
	        return 1E20F;
	} else {
	    if ( s * 1E20F < n )
	        return 1E-20F;
	}
	return s / n;
}

static inline float powerLevel( float snr ) {
	if ( snr >= 1E20 )
	    return 200.0F;
	else if ( snr <= 1E-20 )
	    return -200.0;
	return 10.0F * log10(snr);
}


void	TII_Detector::processNULL_ex (int	*pNumOut,
	                              int	*outTii,
	                              float	*outAvgSNR,
	                              float	*outMinSNR,
	                              float	*outNxtSNR) {
float	Psub[NUM_GROUPS][NUM_SUBIDS];	// power per subID per group
int	Csub[NUM_GROUPS][NUM_SUBIDS];	// subID in 0 .. 23 per group - when sorting powers Psub[][]
float	P_grpPairNoise[NUM_GROUPS];	// avg noise power per carrier-pair
int	numSubIDsInGroup[NUM_GROUPS];	// counter for possible subIDs in group
int	num_subIDs [NUM_SUBIDS];	// counter for possible subIDs over all groups
int		groupNo, i, j;

	*pNumOut = 0;
	if (params.get_dabMode() != 1)
	    return;

//	sum 4 times repeated spectrum of 384 carriers into P_avg[]
//	don't intermix with exponential averaging ..
	static int modeOneCarrierFFTidx [5] = {
	                           (-768 + 2048) % 2048,
	                           (-384 + 2048) % 2048,
	      // center carrier 0 unused!
	      	                   (   1 + 2048) % 2048,
	                           ( 385 + 2048) % 2048,
	                           ( 769 + 2048) % 2048
	};


	memset (P_avg, 0, NUM_TII_EVAL_CARRIERS * sizeof (float));
	for (i = 0; i < 4; i++) {
	   const int k = modeOneCarrierFFTidx [i];
	      for (int j = 0; j < NUM_TII_EVAL_CARRIERS; j++)
	         P_avg [j] += P_allAvg [k + j];
	}

//	initialize num_subIDs []
	memset (num_subIDs, 0, NUM_SUBIDS * sizeof (int));

//	determine possible subIDs per group
	for (groupNo = 0; groupNo < NUM_GROUPS; groupNo++) {
	    const int grpCarrierOff = groupNo * (NUM_SUBIDS * 2);
	    float	* P = &Psub [groupNo][0];
	    int		* C = &Csub [groupNo][0];
	    // sum power for possible carrier pairs per group
	    for ( i = 0; i < NUM_SUBIDS; ++i ) {
	        P [i] = P_avg [grpCarrierOff + 2 * i] +
	                      P_avg [grpCarrierOff + 2 * i + 1];
	        C [i] = grpCarrierOff + 2 * i;
	    }

//	bubble sort NUM_MINS_FOR_NOISE smallest elements to the end
//	using bubble sort cause we don't want to sort all
	   for (j = 0; j < NUM_MINS_FOR_NOISE; j++) {
	        for (i = 0; i < NUM_SUBIDS - 2 - j; i++) {
	            if (P [i] < P [i + 1]) {
	                std::swap (P [i], P [i + 1]);
	                std::swap (C [i], C [i + 1]);
	            }
	        }
	    }

//	avg noise power per carrier-pair - of sorted 4 least powers
	   P_grpPairNoise [groupNo] =
	          std::accumulate (P + (NUM_SUBIDS -NUM_MINS_FOR_NOISE),
	                           P + NUM_SUBIDS, 0.0F ) / NUM_MINS_FOR_NOISE;
	    
//	init counter for possible subIDs in group
	   numSubIDsInGroup[groupNo] = 0;

//	bubble sort MAX_NUM_TII biggest elements to the begin
	   for (j = 0; j < MAX_NUM_TII; j++) {
//	array size is 20, cause lowest 4 elements are
//	already at the tail (of 24 elements)
	      for (i = NUM_SUBIDS - NUM_MINS_FOR_NOISE - 2; i >= j; --i) {
	         if (P [i + 1] > P [i]) {
	            std::swap (P [i], P [i + 1]);
	            std::swap (C [i], C [i + 1]);
	         }
	      }
	      if ((P [j] <  MIN_SNR_POWER_RATIO * P_grpPairNoise [groupNo])
	          || (P_avg [C [j]] < (MIN_SNR_POWER_RATIO / 2.0F) * P_grpPairNoise[groupNo] )
	          || (P_avg [C [j] + 1] < (MIN_SNR_POWER_RATIO / 2.0F) * P_grpPairNoise[groupNo] ))
	         break;
	      numSubIDsInGroup [groupNo] ++;
	      C [j] = (C [j] - grpCarrierOff) / 2; // => C[j] in 0 .. 23
	      num_subIDs [C [j]] ++;
	   }
	}

//	per possible subID .. determine mainID from energy pattern over groups
	for (int subID = 0; subID < NUM_SUBIDS; subID++) {
	   if (num_subIDs [subID] < NUM_SUBIDS_IN_PATTERN )
//	skip if not found minimum 4 times
	        continue;
//
//	So, if we are here, we might have a winner
	   float subP [NUM_GROUPS];	// power of subID in group with subID
	   int	 subGrpNo [NUM_GROUPS];	// groupNo with subID
	   int   numGroupsWithSubID = 0;

	   for (groupNo = 0; groupNo < NUM_GROUPS; groupNo++) {
	      for (i = 0; i < numSubIDsInGroup [groupNo]; i++) {
	         if (Csub [groupNo][i] == subID) {
//	add groups to list of candidates for subID
	            subP [numGroupsWithSubID] = Psub [groupNo][i];
	            subGrpNo [numGroupsWithSubID] = groupNo;
	            numGroupsWithSubID ++;
	            break;
	         }
	      }
	   }

	   if (numGroupsWithSubID < NUM_SUBIDS_IN_PATTERN)
	      continue;

	   int groupPatternBits = 0;
	   float sumPsig	= 0.0F;
	   float sumPnoise	= 0.0F;
	   float maxPnoise	= -1.0F;
	   float minGrpSNR	= - MIN_SNR;
//	bubble sort 4 + 1 maxima to begin
	   for (j = 0; j <= NUM_SUBIDS_IN_PATTERN; j++) {
	      for (i = numGroupsWithSubID - 2; i >= j; i--) {
	         if (subP [i + 1] > subP [i]) {
	            std::swap (subP [i], subP [i + 1]);
	            std::swap (subGrpNo [i], subGrpNo [i + 1]);
	         }
	      }

	      if (j < NUM_SUBIDS_IN_PATTERN) {
	         groupNo = subGrpNo [j];
	         groupPatternBits |= ( 1 << ( NUM_GROUPS -1 - groupNo));
	         sumPsig	+= subP [j];
	         sumPnoise	+= P_grpPairNoise [groupNo];
	         if (P_grpPairNoise [groupNo] > maxPnoise)
	            maxPnoise = P_grpPairNoise [groupNo];
#if 1
	         float grpSNR =
	                 powerRatio (subP [j], P_grpPairNoise [groupNo]);
	         if ((grpSNR < minGrpSNR) || (minGrpSNR < 0.0F))
	            minGrpSNR = grpSNR;
#endif
	      }
	   }

	   const float minPsig = subP [NUM_SUBIDS_IN_PATTERN - 1];
//	output if above minimum SNR
	   float minSNR = powerRatio (minPsig, maxPnoise);
//	float minSNR = minGrpSNR;

	   if (minSNR >= MIN_SNR_POWER_RATIO) {
//	      outTii [*pNumOut] = 1000 * groupPatternBits + subID;
	      outTii [*pNumOut] =
	             100 * int (invTable [groupPatternBits]) + subID;
	      float avgSNR	= powerRatio (sumPsig, sumPnoise);
	      float nextMainSNR = MAX_SNR;
//	max + 200 dB as default, if there is no other group
	      if (numGroupsWithSubID > NUM_SUBIDS_IN_PATTERN)
	         nextMainSNR =
	                   powerRatio (subP [NUM_SUBIDS_IN_PATTERN],
	                    P_grpPairNoise [subGrpNo [NUM_SUBIDS_IN_PATTERN]]);

	      outAvgSNR [*pNumOut] = powerLevel (avgSNR);
//	      outMinSNR [*pNumOut] = powerLevel (minSNR);
	      outMinSNR [*pNumOut] = powerLevel (minGrpSNR);
	      outNxtSNR [*pNumOut] = powerLevel (nextMainSNR);
	      (*pNumOut) ++;
	   }
	}
}


//
//

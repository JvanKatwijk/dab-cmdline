#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the Qt-DAB program
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"tii_detector.h"

#include <utility>
#include <numeric>

#include	<stdio.h>
#include	<inttypes.h>

// configure / adapt these
#define MIN_SNR_POWER_RATIO		3.0F	/* 3.0 == 4.7 dB */
#define MAX_NUM_TII				7		/* evaluate up to 7 TII-IDs */
#define NUM_MINS_FOR_NOISE		4		/* use lowest 4 carrier pairs to determine noise level */

#define OUT_STAT			0
#define LOG_WIN_SUM			0


// NOT for configuration - just avoid magic numbers in code
#define NUM_GROUPS				8
#define NUM_SUBIDS				24
#define NUM_TII_EVAL_CARRIERS	( NUM_GROUPS * NUM_SUBIDS * 2 )		/* = 384 */
#define NUM_SUBIDS_IN_PATTERN	4		/* 8 groups in total. but only 4 active */
#define MIN_SNR				1E-20F	/* -200 dB */
#define MAX_SNR				1E20F	/* +200 dB */


//
//	Transmitter Identification Info is carrier in the null period
//	of a DAB frame. In case the FIB's carry information on the
//	set of transmitters used in the SFN, an attempt is made
//	to identify the transmitter by inspecting the null period.
//	The information in the null-period is encoded in a "p"
//	a "pattern" and a "c", a "carrier"
//	value. The "p" value, derived from the FIB, defines the
//	pattern within the null-period as well as a set of
//	startcarriers, i.e. carrier numbers where the pattern
//	could start.
//	The start carrier itself determined the "c" value.
//	Basically, within an SFN the "p" is fixed for all transmitters,
//	while the latter show the pattern on different positions in
//	the carriers of the null-period.
//
//	Matching the position of the pattern is relatively easy, since
//	the standard defines the signals (i.e. phase and amplitude) of
//	the carriers in the pattern.
//
//	As it turns out, the pattern is represented by a sequence
//	consisting of elements with two subsequent bins with the same
//	value, followed by a "gap" of K * 48 (-1) bins.
//
//	The constructor of the class generates the patterns, according
//	to the algorithm in the standard.

// forward declaration
static void initInvTable();


		TII_Detector::TII_Detector (uint8_t dabMode):
	                                          phaseTable (dabMode),
	                                          params (dabMode),
	                                          my_fftHandler (dabMode) {
int16_t	i;
float	Phi_k;
double	winSum = 0.0;
double	winElem;

	this	-> T_u		= params. get_T_u ();
	carriers		= params. get_carriers ();
	theBuffer. resize	(T_u);

	fillCount		= 0;
	fft_buffer		= my_fftHandler. getVector ();	
	isFirstAdd = true;
	numUsedBuffers = 0;

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

        refTable.               resize (T_u);

        memset (refTable. data (), 0, sizeof (std::complex<float>) * T_u);
        for (i = 1; i <= params. get_carriers () / 2; i ++) {
           Phi_k =  get_Phi (i);
           refTable [T_u / 2 + i] = std::complex<float> (cos (Phi_k), sin (Phi_k));
           Phi_k = get_Phi (-i);
           refTable [T_u / 2 - i] = std::complex<float> (cos (Phi_k), sin (Phi_k));
        }

	createPattern (dabMode);

	initInvTable();
}

		TII_Detector::~TII_Detector (void) {
}

//	Zm (0, k) is a function of the P and the C, together forming the key to
//	the database where he transmitter locations are described.
//
//	p is in the range  0 .. 69
//	c is in the range 0 .. 23
//	The transmitted signal - for a given p and c -
//	   Zm (0, k) = A (c, p) (k) e^jPhi (k) + A (c, p) (k - 1) e ^jPhi (k - 1)
//
//	a (b, p) = getBit (table [p], b);

// groupPattern_bitset = table[ mainId ]
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


// groupPattern_bitset = table[ mainId ]
// mainId = invTable[ groupPattern_bitset ]
static
uint8_t invTable [256];

static
void initInvTable() {
	int	i;
	for (i = 0; i < 256; ++i)
	    invTable[i] = 100;		// initialize with an invalid mainId!
	for (i = 0; i < 70; ++i)
	    invTable[ table[i] ] = i;
}


static
uint8_t bits [] = {128, 64, 32, 16, 8, 4, 2, 1};
static inline
uint8_t getbit (uint8_t value, int16_t bitPos) {
	return value & bits [bitPos] ? 1 : 0;
}

static inline
uint8_t delta (int16_t a, int16_t b) {
	return a == b ? 1 : 0;
}
//
//	Litterally copied from the standard. There is no doubt that
//	creating the patterns above can be done more efficient, however
//	this approach feels more readable, while initialization, i.e.
//	executing the constructor, is done once
int16_t	TII_Detector::A (uint8_t dabMode, uint8_t c, uint8_t p, int16_t k) {
	if (dabMode == 2)
	   return A_mode_2 (c, p, k);
	if (dabMode == 4)
	   return A_mode_4 (c, p, k);
	return A_mode_1 (c, p, k);
}

int16_t	TII_Detector::A_mode_2 (uint8_t c, uint8_t p, int16_t k) {
int16_t	res	= 0;
int16_t	b;

	if ((-192 <= k) && (k < - 191)) {
	   for (b = 0; b <= 3; b ++)
	      res += delta (k, -192 + 2 * c + 48 * b) * getbit (table [p], b);
	   for (b = 4; b <= 7; b ++)
	      res += delta (k, -191 + 2 * c + 48 * b) * getbit (table [p], b);
	}
	return res;
}

int16_t	TII_Detector::A_mode_4 (uint8_t c, uint8_t p, int16_t k) {
	return 0;	// avoid compiler warning
}

int16_t	TII_Detector::A_mode_1 (uint8_t c, uint8_t p, int16_t k) {
int16_t b;
int16_t res	= 0;

	if ((-768 <= k) && (k < - 384))
	   for (b = 0; b <= 7; b ++)
	      res += delta (k, -768 + 2 * c + 48 * b) * getbit (table [p], b);
	else
	if ((-384 <= k) && (k < 0))
	   for (b = 0; b <= 7; b ++)
	      res += delta (k, -384 + 2 * c + 48 * b) * getbit (table [p], b);
	else
	if ((0 < k) && (k <= 384))
	   for (b = 0; b <= 7; b ++)
	      res += delta (k, 1 + 2 * c + 48 * b) * getbit (table [p], b);
	else
	if ((384 < k) && (k <= 768))
	   for (b = 0; b <= 7; b ++)
	      res += delta (k, 385 + 2 * c + 48 * b) * getbit (table [p], b);
	else
	   return 0;

	return res;
}
//
//	If we know the "mainId" from the FIG0/22, we can try to locate
//	the pattern and compute the C

void	TII_Detector::reset (void) {
	isFirstAdd = true;
	numUsedBuffers = 0;
}

//	To eliminate (reduce?) noise in the input signal, we might
//	add a few spectra before computing (up to the user)
void	TII_Detector::addBuffer (std::vector<std::complex<float>> v, float alfa, int32_t cifCounter) {
int	i;

	// apply reset()
	if (isFirstAdd) {
	   memset (theBuffer. data (), 0, T_u * sizeof (std::complex<float>));
	   memset (&P_allAvg[0], 0, 2048 * sizeof(float));
	}

#if OUT_STAT
	float timeMaxNorm = std::norm( v[0] );
	for ( int j = 1; j < T_u; ++j ) {
	   float n = std::norm( v[j] );
	   if ( n > timeMaxNorm )
	      timeMaxNorm = n;
	}
#endif

	// windowing + FFT
	for (i = 0; i < T_u; i ++)
	   fft_buffer [i] = cmul (v [i], window [i]);
	my_fftHandler. do_FFT ();


	for ( int j = 0; j < T_u; ++j ) {
	   P_tmpNorm[j] = std::norm( fft_buffer[j] );
	}

#if OUT_STAT
	float specMaxNorm = std::norm( P_tmpNorm[0] );
	for ( int j = 1; j < T_u; ++j ) {
	   if ( P_tmpNorm[j] > specMaxNorm )
	      specMaxNorm = P_tmpNorm[j];
	}

	float timeMag = sqrt( timeMaxNorm );
	float specMag = sqrt( specMaxNorm );
	fprintf(stderr, "TII_Detector addBuffer(): %d / %u: timeMax = %f  specMax = %f\n", cifCounter, numUsedBuffers, timeMag, specMag);
	
	if ( specMaxNorm > (32768.0F) ) {	// must be an error!  ( 128*128 *2 )
	   fprintf(stderr, "TII_Detector addBuffer(): OVERFLOW! %d / %u: max norm() = %f\n", cifCounter, numUsedBuffers, specMaxNorm );
	   return;		// do NOT use this spectrum
	}
#endif

	if ( alfa < 0.0F || isFirstAdd ) {
	    for ( int j = 0; j < T_u; ++j )
	        P_allAvg[j] += P_tmpNorm[j];
	    for (i = 0; i < T_u; i ++)
	        theBuffer [i] += fft_buffer [(T_u / 2 + i) % T_u];
	    isFirstAdd = false;
	    ++numUsedBuffers;
	} else {
	    const float alpha = alfa;
	    const float beta = 1.0F - alpha;
	    for ( int j = 0; j < T_u; ++j )
	        P_allAvg[j] = alpha * P_allAvg[j] + beta * P_tmpNorm[j];
	    for (i = 0; i < T_u; i ++)
	        theBuffer [i] = alpha * theBuffer [i] + beta * fft_buffer [(T_u / 2 + i) % T_u];
	    ++numUsedBuffers;
	}
}

//	We collected some  "spectra', and start correlating the 
//	combined spectrum with the pattern defined by the mainId

int16_t	TII_Detector::find_C (int16_t mainId) {
int16_t	i;
int16_t	startCarrier	= theTable [mainId]. carrier;
uint64_t pattern	= theTable [mainId]. pattern;
float	maxCorr		= -1;
int	maxIndex	= -1;
float	avg		= 0;
float	corrTable [24];

	if (mainId < 0)
	   return - 1;

//	fprintf (stderr, "the carrier is %d\n", startCarrier);
//
//	The "c" value is in the range 1 .. 23
	for (i = 1; i < 24; i ++) {
	   corrTable [i] = correlate (theBuffer,
	                              startCarrier + 2 * i,
	                              pattern);
	}

	for (i = 1; i < 24; i ++) {
	   avg += corrTable [i];
	   if (corrTable [i] > maxCorr) {
	      maxCorr = corrTable [i];
	      maxIndex = i;
	   }
	}

	avg /= 23;
	if (maxCorr < 2 * avg)
	   maxIndex = -1;

	return maxIndex;
}
//
//	The difficult one: guessing for Mode 1 only
void	TII_Detector::processNULL (int16_t *mainId, int16_t *subId) {
int i, j;
float   maxCorr_1	= 0;
float   maxCorr_2	= 0;
float   avg		= 0;
int16_t	startCarrier	= -1;
int16_t	altCarrier	= -1;

        for (i = - carriers / 2; i < - carriers / 4; i ++)
           avg += abs (real (theBuffer [T_u / 2 + i] *
                                    conj (theBuffer [T_u / 2 + i + 1])));
        avg /= (carriers / 4);

	for (i = - carriers / 2; i < - carriers / 2 + 4 * 48; i += 2) {
	   int index = T_u / 2 + i;
	   float sum_1 = 0;
	   float sum_2 = 0;
//
//	  5 * avg is a rather arbitrary threshold, sometimes we see
//	  values of 100 * avg
	   if (abs (real (theBuffer [index] *
	                     conj (theBuffer [index + 1]))) < 5 * avg)
	      continue;

//	We just compute the "best" candidates two ways

	   for (j = 0; j < 4 * 8; j ++) {
	      int ci = index + j * 48;
	      if (ci >= T_u / 2)
	         ci ++;
	      sum_1 += abs (real (theBuffer [ci] * conj (theBuffer [ci + 1])));
	      sum_2 += abs (real (theBuffer [ci] * conj (refTable  [ci]))) +
	               abs (real (theBuffer [ci + 1] * conj (refTable [ci])));
	   }

	   if (sum_1 > maxCorr_1) {
	      maxCorr_1	= sum_1;
	      startCarrier = i;
	   }

	   if (sum_2 > maxCorr_2) {
	      maxCorr_2 = sum_2;
	      altCarrier = i;
	   }
	}

	if (startCarrier != altCarrier) {
//	   fprintf (stderr, "alternative start carriers %d %d\n",
//	                                        startCarrier, altCarrier);
	                                   
	   return;
	}

        if (startCarrier <  -carriers / 2)	// nothing found
           return;
//	fprintf (stderr, "startCarrier is %d, altCarrier %d\n",
//	                         startCarrier, altCarrier);


	float  maxCorr = -1;
        *mainId = -1;
        *subId  = -1;

        for (i = 0; i < 70; i ++) {
           if ((startCarrier < theTable [i]. carrier) ||
               (theTable [i]. carrier + 48 <= startCarrier))
              continue;
           float corr = correlate (theBuffer,
                                   startCarrier,
                                   theTable [i]. pattern);
           if (corr > maxCorr) {
              maxCorr = corr;
              *mainId   = i;
              *subId    = (startCarrier - theTable [i]. carrier) / 2;
           }
        }

//	if (*mainId != -1)
//	   fprintf (stderr, "(%d) the carrier is %d, the pattern is %llx\n",
//	                                 *mainId,
//                                         startCarrier,
//	                                 theTable [*mainId]. pattern);
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


void	TII_Detector::processNULL_ex (int *pNumOut, int *outTii, float *outAvgSNR, float *outMinSNR, float *outNxtSNR) {
	float	Psub[NUM_GROUPS][NUM_SUBIDS];	// power per subID per group
	int		Csub[NUM_GROUPS][NUM_SUBIDS];	// subID in 0 .. 23 per group - when sorting powers Psub[][]
	float	P_grpPairNoise[NUM_GROUPS];		// avg noise power per carrier-pair
	int		numSubIDsInGroup[NUM_GROUPS];	// counter for possible subIDs in group
	int		num_subIDs[NUM_SUBIDS];			// counter for possible subIDs over all groups
	int		groupNo, i, j;

	*pNumOut = 0;
	if ( params.get_dabMode() != 1 )
	    return;

	// sum 4 times repeated spectrum of 384 carriers into P_avg[]
	//   don't intermix with exponential averaging ..
	{
	   static int modeOneCarrierFFTidx[5] = {
	      (-768 +2048) % 2048,
	      (-384 +2048) % 2048,
	      // center carrier 0 unused!
	      (1 +2048) % 2048,
	      (385 +2048) % 2048,
	      (769 +2048) % 2048
	   };

	   for ( i = 0; i < 1; ++i ) {
	      const int k = modeOneCarrierFFTidx[i];
	      for ( int j = 0; j < NUM_TII_EVAL_CARRIERS; ++j )
	         P_avg[j] = P_allAvg[k + j];
	   }
	   for ( i = 1; i < 4; ++i ) {
	      const int k = modeOneCarrierFFTidx[i];
	      for ( int j = 0; j < NUM_TII_EVAL_CARRIERS; ++j )
	         P_avg[j] += P_allAvg[k + j];
	   }
	}

	// initialize num_subIDs[]
	for ( i = 0; i < NUM_SUBIDS; ++i )
	    num_subIDs[i] = 0;

	// determine possible subIDs per group
	for ( groupNo = 0; groupNo < NUM_GROUPS; ++groupNo ) {
	    const int grpCarrierOff = groupNo * ( NUM_SUBIDS * 2 );
	    float	* P = &Psub[groupNo][0];
	    int		* C = &Csub[groupNo][0];
	    // sum power for possible carrier pairs per group
	    for ( i = 0; i < NUM_SUBIDS; ++i ) {
	        P[i] = P_avg[ grpCarrierOff +2*i ] + P_avg[ grpCarrierOff +2*i +1 ];
	        C[i] = grpCarrierOff +2*i;
	    }
	    // bubble sort 4 smallest elements to the end
	    //   using bubble sort cause we don't want to sort all
	    for ( j = 0; j < NUM_MINS_FOR_NOISE; ++j ) {
	        for ( i = 0; i < NUM_SUBIDS -2 -j; ++i ) {
	            if ( P[i] < P[i+1] ) {
	                std::swap( P[i], P[i+1] );
	                std::swap( C[i], C[i+1] );
	            }
	        }
	    }
	    // avg noise power per carrier-pair - of sorted 4 least powers
	    P_grpPairNoise[groupNo] = std::accumulate( P +(NUM_SUBIDS -NUM_MINS_FOR_NOISE), P +NUM_SUBIDS, 0.0F ) / NUM_MINS_FOR_NOISE;
	    
	    numSubIDsInGroup[groupNo] = 0;	// init counter for possible subIDs in group
	    // bubble sort 7 biggest elements to the begin
	    for ( j = 0; j < MAX_NUM_TII; ++j ) {
	        // array size is 20, cause lowest 4 elements are already at the tail (of 24 elements)
	        for ( i = NUM_SUBIDS -NUM_MINS_FOR_NOISE -2; i >= j; --i ) {
	            if ( P[i+1] > P[i] ) {
	                std::swap( P[i], P[i+1] );
	                std::swap( C[i], C[i+1] );
	            }
	        }
	        if ( ( P[j] <  MIN_SNR_POWER_RATIO * P_grpPairNoise[groupNo] )
	          || ( P_avg[ C[j]    ] < (MIN_SNR_POWER_RATIO / 2.0F) * P_grpPairNoise[groupNo] )
	          || ( P_avg[ C[j] +1 ] < (MIN_SNR_POWER_RATIO / 2.0F) * P_grpPairNoise[groupNo] ) )
	            break;
	        ++numSubIDsInGroup[groupNo];
	        C[j] = ( C[j] - grpCarrierOff ) / 2;	// => C[j] in 0 .. 23
	        ++num_subIDs[ C[j] ];
	    }
	}

	// per possible subID .. determine mainID from energy pattern over groups
	for ( int subID = 0; subID < NUM_SUBIDS; ++subID ) {
	    if ( num_subIDs[subID] < NUM_SUBIDS_IN_PATTERN )	// skip if not found minimum 4 times
	        continue;

	    float	subP[NUM_GROUPS];			// power of subID in group with subID
	    int		subGrpNo[NUM_GROUPS];		// groupNo with subID
	    int		numGroupsWithSubID = 0;

	    for ( groupNo = 0; groupNo < NUM_GROUPS; ++groupNo ) {
	        for ( i = 0; i < numSubIDsInGroup[groupNo]; ++i ) {
	            if ( Csub[groupNo][i] == subID ) {
	                // add groups to list of candidates for subID
	                subP[numGroupsWithSubID] = Psub[groupNo][i];
	                subGrpNo[numGroupsWithSubID] = groupNo;
	                ++numGroupsWithSubID;
	                break;
	            }
	        }
	    }

	    if ( numGroupsWithSubID < NUM_SUBIDS_IN_PATTERN )
	        continue;

	    int groupPatternBits = 0;
	    float sumPsig = 0.0F;
	    float sumPnoise = 0.0F;
	    float maxPnoise = -1.0F;
	    float minGrpSNR = - MIN_SNR;
	    // bubble sort 4 + 1 maxima to begin
	    for ( j = 0; j <= NUM_SUBIDS_IN_PATTERN; ++j ) {
	        for ( i = numGroupsWithSubID -2; i >= j; --i ) {
	            if ( subP[i+1] > subP[i] ) {
	                std::swap( subP[i], subP[i+1] );
	                std::swap( subGrpNo[i], subGrpNo[i+1] );
	            }
	        }
	        if ( j < NUM_SUBIDS_IN_PATTERN ) {
	            groupNo = subGrpNo[j];
	            groupPatternBits |= ( 1 << ( NUM_GROUPS -1 -groupNo ) );
	            sumPsig += subP[j];
	            sumPnoise += P_grpPairNoise[ groupNo ];
	            if ( P_grpPairNoise[ groupNo ] > maxPnoise )
	                maxPnoise = P_grpPairNoise[ groupNo ];
#if 1
                float grpSNR = powerRatio( subP[j], P_grpPairNoise[ groupNo ] );
	            if ( grpSNR < minGrpSNR || minGrpSNR < 0.0F )
	                minGrpSNR = grpSNR;
#endif
	        }
	    }
	    const float minPsig = subP[NUM_SUBIDS_IN_PATTERN -1];

	    // output if above minimum SNR
	    float minSNR = powerRatio( minPsig, maxPnoise );
	    //float minSNR = minGrpSNR;
	    if ( minSNR >= MIN_SNR_POWER_RATIO ) {
	
	        // outTii[ *pNumOut ] = 1000 * groupPatternBits + subID;
	        outTii[ *pNumOut ] = 100 * int(invTable[ groupPatternBits ]) + subID;
	        
	        float avgSNR = powerRatio( sumPsig, sumPnoise );
	        
	        float nextMainSNR = MAX_SNR;	// max +200 dB as default, if there is no other group
	        if ( numGroupsWithSubID > NUM_SUBIDS_IN_PATTERN )
	            nextMainSNR = powerRatio( subP[NUM_SUBIDS_IN_PATTERN], P_grpPairNoise[ subGrpNo[NUM_SUBIDS_IN_PATTERN] ] );

	        outAvgSNR[ *pNumOut ] = powerLevel(avgSNR);
	        //outMinSNR[ *pNumOut ] = powerLevel(minSNR);
	        outMinSNR[ *pNumOut ] = powerLevel(minGrpSNR);
	        outNxtSNR[ *pNumOut ] = powerLevel(nextMainSNR);
	        ++(*pNumOut);
	    }
	}
}


//
//
//	It turns out that the location "startIndex + k * 48"
//	and "startIndex + k * 48 + 1" both contain the refTable
//	value from "startIndex + k * 48" (where k is 1 .. 5, determined
//	by the pattern). Since there might be a pretty large
//	phase difference between the values in the spectrum of the
//	null period here and the values in the predefined refTable,
//	we just correlate  here over the values here
//
float	TII_Detector::correlate (std::vector<complex<float>> v,
	                         int16_t	startCarrier,
	                         uint64_t	pattern) {
int16_t	realIndex;
int16_t	i;
int16_t	carrier;
float	s1	= 0;

	if (pattern == 0)
	   return 0;

	carrier		= startCarrier;
	s1		= abs (real (v [T_u / 2 + startCarrier] *
	                             conj (v [T_u / 2 + startCarrier + 1])));
	for (i = 0; i < 15; i ++) {
	   carrier	+= ((pattern >> 56) & 0xF) * 48;
	   realIndex	= carrier < 0 ? T_u / 2 + carrier :  T_u / 2 + carrier + 1;
	   float x	= abs (real (v [realIndex] *
	                                   conj (v [realIndex + 1])));
	   s1 += x;
	   pattern <<= 4;
	}
	
	return s1 / 15;
}

void	TII_Detector:: createPattern (uint8_t dabMode) {
	switch (dabMode) {
	   default:
	   case 1:
	      createPattern_1 ();
	      break;

	   case 2:
	      createPattern_2 ();
	      break;

	   case 4:
	      createPattern_4 ();
	      break;
	}
}

void	TII_Detector::createPattern_1 (void) {
int	p, k, c;
uint8_t	dabMode	= 1;

	for (p = 0; p < 70; p ++) {
	   int16_t digits	= 0;
	   c = 0;		// patterns are equal for all c values
	   {
	      bool first = true;
	      int lastOne = 0;
	      for (k = -carriers / 2; k < -carriers / 4; k ++) {
	         if (A (dabMode, c, p, k) > 0) {
	            if (first) {
	               first = false;
	               theTable [p]. carrier = k;
	               theTable [p]. pattern = 0;
	            }
	            else {
	               theTable [p]. pattern <<= 4;
	               theTable [p]. pattern |= (k - lastOne) / 48;
	               digits ++;
	            }
	            lastOne = k;
	         }
	      }

	      for (k = -carriers / 4; k < 0; k ++) {
	         if (A (dabMode, c, p, k) > 0) {
	            theTable [p]. pattern <<= 4;
	            theTable [p]. pattern |= (k - lastOne) / 48;
	            lastOne = k;
	            digits ++;
	         }
	      }

	      for (k = 1; k <= carriers / 4; k ++) {
	         if (A (dabMode, c, p, k) > 0) {
	            theTable [p]. pattern <<= 4;
	            theTable [p]. pattern |= (k - lastOne) / 48;
	            lastOne = k;
	            digits ++;
	         }
	      }

	      for (k = carriers / 4 + 1; k <= carriers / 2; k ++) {
	         if (A (dabMode, c, p, k) > 0) {
	            theTable [p]. pattern <<= 4;
	            theTable [p]. pattern |= (k - lastOne) / 48;
	            lastOne = k;
	            digits ++;
	         }
	      }
	   }
//	   fprintf (stderr, "p = %d\tc = %d\tk=%d\tpatter=%llX (digits = %d)\n",
//	                     p, c, theTable [p]. carrier,
//	                        theTable [p]. pattern, digits);
	}
}


void	TII_Detector::createPattern_2 (void) {
}

void	TII_Detector::createPattern_4 (void) {
}


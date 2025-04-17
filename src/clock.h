/*******************************************************************************
** clock.h (synth clock rate)
*******************************************************************************/

#ifndef CLOCK_H
#define CLOCK_H

/* the sampling rate is set to be high enough so  */
/* that B9 (the highest pitch in the frequency    */
/* table) is still under the Nyquist when we use  */
/* the highest tuning fork, which is A444.        */

/* additionally, we want the sampling rate to be  */
/* a multiple of 1000, so each millisecond has an */
/* integer number of samples.                     */

#define CLOCK_SAMPLING_RATE 32000

#endif

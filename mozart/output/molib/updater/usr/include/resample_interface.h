/**
 * @file resample_interface.h
 * @brief For the operation to resample
 * @version 1.0.0
 * @date 2016-04-29
 *
 * Copyright (C) 2016 Ingenic Semiconductor Co., Ltd.
 *
 * The program is not free, Ingenic without permission,
 * no one shall be arbitrarily (including but not limited
 * to: copy, to the illegal way of communication, display,
 * mirror, upload, download) use, or by unconventional
 * methods (such as: malicious intervention Ingenic data)
 * Ingenic's normal service, no one shall be arbitrarily by
 * software the program automatically get Ingenic data
 * Otherwise, Ingenic will be investigated for legal responsibility
 * according to law.
 */

/*resample_interface.h*/
#ifndef _RESAMPLE_INTERFACE_H_
#define _RESAMPLE_INTERFACE_H_

#ifdef LAVCRESAMPLE
#define AF_NCH 8
typedef struct af_resample_s{
    struct AVResampleContext *avrctx;
    int16_t *in[AF_NCH];
    int in_alloc;
    int index;
	int in_rate;
	int out_rate;

    int filter_length;
    int linear;
    int phase_shift;
    double cutoff;
}af_resample_t;
#else
typedef struct af_resample_s
{
	void* w;			// Current filter weights
	void** xq; 			// Circular buffers
	unsigned int xi; 		// Index for circular buffers
	unsigned int wi;		// Index for w
	unsigned int i; 		// Number of new samples to put in x queue
	unsigned int dn;		// Down sampling factor
	unsigned int up;		// Up sampling factor
	unsigned long long step;	// Step size for linear interpolation
	unsigned long long pt;		// Pointer remainder for linear interpolation
} af_resample_t;
#endif

#ifdef  __cplusplus
extern "C" {
#endif
	/**
	 * @in_rate: sample rate before resample
	 * @out_rate: sample rate after resample
	 * @bps: bit width in bytes
	 *
	 * @brief: Design the filter and init
	 *
	 * @return: On succes sreturn the af_resample_t* , and return NULL if an error occurred
	 */
	extern af_resample_t *mozart_resample_init(unsigned int in_rate, unsigned int channel,
						   unsigned int bps, unsigned int out_rate);

	/**
	 * @brief: Deallocate memory
	 */
	extern void mozart_resample_uninit(af_resample_t* s);

	/**
	 * @inputlen: data len to resample
	 * @in_rate: sample rate before resample
	 * @channel: count of channel
	 * @bitspersample: sample width in bits
	 * @out_rate: sample rate after resample
	 * @return: On succes return data len after reample, and return 0 if an error occurred
	 */
	extern unsigned int mozart_resample_get_outlen(unsigned int inputlen, unsigned int in_rate, unsigned int channel, unsigned int bitspersample, unsigned int out_rate);
	/*
	 * @bitspersample: bit width in bits, only support for 16bits(short) and 32bits(float)
	 * @inbuff: data input buff before resample
	 * @inputlen: data len of input
	 * @outbuff: data out buffer after resample
	 *
	 * @brief: Resample
	 *
	 * @return: On success return the data len after resample, and return 0 if an error occurred
	 */
	extern unsigned int mozart_resample(af_resample_t* s, unsigned int channel, unsigned int bitspersample,
					    char* inbuff, unsigned int inputlen, char* outbuff);

#ifdef  __cplusplus
}
#endif

#endif //_RESAMPLE_INTERFACE_H_


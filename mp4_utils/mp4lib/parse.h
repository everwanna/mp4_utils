#pragma once

#include "moov.h"

unsigned int stco_get_entries(unsigned char const* stco);
unsigned int stco_get_offset(unsigned char const* stco, int idx);
unsigned int stsc_get_entries(unsigned char const* stsc);
unsigned int stsz_get_sample_size(unsigned char const* stsz);
void stsc_get_table(unsigned char const* stsc, unsigned int i, struct stsc_table_t *stsc_table);
unsigned int stsz_get_entries(unsigned char const* stsz);
unsigned int stsz_get_size(unsigned char const* stsz, unsigned int idx);

void stts_get_sample_count_and_duration(unsigned char const* stts,
										unsigned int idx, unsigned int* sample_count, unsigned int* sample_duration);

unsigned int stts_get_entries(unsigned char const* stts);

void ctts_get_sample_count_and_offset(unsigned char const* ctts,
									  unsigned int idx, unsigned int* sample_count, unsigned int* sample_offset);

unsigned int ctts_get_entries(unsigned char const* ctts);

void mdia_parse(struct mdia_t* mdia, unsigned char* buffer, unsigned int size);

void trak_write_index(trak* trak_atom, unsigned int start, unsigned int end);

uint32_t get_sample_duration( trak * trak_atom );
long mdhd_get_time_scale(unsigned char* mdhd);
long mvhd_get_time_scale(unsigned char* mvhd);

uint32_t mvhd_get_duration(unsigned char* mvhd);

void mdhd_set_duration(unsigned char* mdhd, unsigned int duration);
void tkhd_set_duration(unsigned char* tkhd, unsigned int duration);
void mvhd_set_duration(unsigned char* mvhd, long duration);

int file_atom_read_header(FILE* infile, atom * atom);

unsigned int stts_get_sample(unsigned char const* stts, unsigned int time);

unsigned int stbl_get_nearest_keyframe(struct stbl_t const* stbl, unsigned int sample);

unsigned int stts_get_samples(unsigned char const* stts);

unsigned int ctts_get_samples(unsigned char const* ctts);
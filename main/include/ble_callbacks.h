#pragma once

#include "parser.h"

bool on_pairing_msg(void *ctx, mfg_data_t *mfg);
bool on_lost_msg(void *ctx, mfg_data_t *mfg);
bool on_paired_msg(void *ctx, mfg_data_t *mfg);

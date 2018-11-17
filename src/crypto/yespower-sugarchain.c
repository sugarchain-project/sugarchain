// Copyright (c) 2018 The Sugarchain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdlib.h>
#include "yespower-1.0.0/yespower.h"

static const yespower_params_t yespower_0_5_original = {YESPOWER_0_5, 2048, 8, "Client Key", 10};
static const yespower_params_t yespower_1_0_sugarchain = {YESPOWER_1_0, 2048, 32, NULL, 0};

void yespower_hash(const char *input, char *output){
  if (yespower_tls(input, 80, &yespower_1_0_sugarchain, (yespower_binary_t *) output))
      abort();
}

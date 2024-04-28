#pragma once
#define safe_xgboost(call)                                                   \
  {                                                                          \
    int err = (call);                                                        \
    if (err != 0) {                                                          \
      fprintf(stderr, "%s:%d: error in %s: %s\n", __FILE__, __LINE__, #call, \
              XGBGetLastError());                                            \
      exit(1);                                                               \
    }                                                                        \
  }
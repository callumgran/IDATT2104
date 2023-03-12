#ifndef BASE64_H
#define BASE64_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <openssl/bio.h>
#include <openssl/pem.h>

int base64_encode(char *in_str, int in_len, char *out_str);

#endif
#ifndef BASE64_H
#define BASE64_H

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length);

char *base64_encode2(const void *buf, size_t size);

#endif
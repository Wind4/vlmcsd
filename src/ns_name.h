
#ifndef NS_NAME_H_
#define NS_NAME_H_

int
ns_name_uncompress_vlmcsd(const uint8_t *msg, const uint8_t *eom, const uint8_t *src,
		   char *dst, size_t dstsiz);

#endif /* NS_NAME_H_ */

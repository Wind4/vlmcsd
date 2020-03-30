
#ifndef NS_PARSE_H_
#define NS_PARSE_H_

#ifndef NS_INT16SZ
#define NS_INT16SZ (sizeof(uint16_t))
#endif // NS_INT16SZ

#ifndef NS_INT32SZ
#define NS_INT32SZ (sizeof(uint32_t))
#endif // NS_INT32SZ

#ifndef NS_MAXDNAME
#define NS_MAXDNAME 1025
#endif

#define ns_msg_id_vlmcsd(handle) ((handle)._id + 0)
#define ns_msg_base_vlmcsd(handle) ((handle)._msg + 0)
#define ns_msg_end_vlmcsd(handle) ((handle)._eom + 0)
#define ns_msg_size_vlmcsd(handle) ((handle)._eom - (handle)._msg)
#define ns_msg_count_vlmcsd(handle, section) ((handle)._counts[section] + 0)

#define ns_rr_name_vlmcsd(rr)	(((rr).name[0] != '\0') ? (rr).name : ".")
#define ns_rr_type_vlmcsd(rr)	((ns_type)((rr).type + 0))
#define ns_rr_class_vlmcsd(rr)	((ns_class)((rr).rr_class + 0))
#define ns_rr_ttl_vlmcsd(rr)	((rr).ttl + 0)
#define ns_rr_rdlen_vlmcsd(rr)	((rr).rdlength + 0)
#define ns_rr_rdata_vlmcsd(rr)	((rr).rdata + 0)

#define ns_msg_id_vlmcsd(handle) ((handle)._id + 0)
#define ns_msg_base_vlmcsd(handle) ((handle)._msg + 0)
#define ns_msg_end_vlmcsd(handle) ((handle)._eom + 0)
#define ns_msg_size_vlmcsd(handle) ((handle)._eom - (handle)._msg)
#define ns_msg_count_vlmcsd(handle, section) ((handle)._counts[section] + 0)


typedef enum __ns_sect_vlmcsd {
	ns_s_qd_vlmcsd = 0,		/*%< Query: Question. */
	ns_s_zn_vlmcsd = 0,		/*%< Update: Zone. */
	ns_s_an_vlmcsd = 1,		/*%< Query: Answer. */
	ns_s_pr_vlmcsd = 1,		/*%< Update: Prerequisites. */
	ns_s_ns_vlmcsd = 2,		/*%< Query: Name servers. */
	ns_s_ud_vlmcsd = 2,		/*%< Update: Update. */
	ns_s_ar_vlmcsd = 3,		/*%< Query|Update: Additional records. */
	ns_s_max_vlmcsd = 4
} ns_sect_vlmcsd;

typedef struct __ns_msg_vlmcsd {
	uint8_t	*_msg, *_eom;
	uint16_t	_id, _flags, _counts[ns_s_max_vlmcsd];
	uint8_t	*_sections[ns_s_max_vlmcsd];
	ns_sect_vlmcsd		_sect;
	int		_rrnum;
	uint8_t	*_msg_ptr;
} ns_msg_vlmcsd;


typedef	struct __ns_rr_vlmcsd {
	char		name[NS_MAXDNAME];
	uint16_t	type;
	uint16_t	rr_class;
	uint32_t	ttl;
	uint16_t	rdlength;
	uint8_t *	rdata;
} ns_rr_vlmcsd;

int ns_initparse_vlmcsd(uint8_t *msg, int msglen, ns_msg_vlmcsd *handle);

int ns_parserr_vlmcsd(ns_msg_vlmcsd *handle, ns_sect_vlmcsd section, int rrnum, ns_rr_vlmcsd *rr);



#endif /* NS_PARSE_H_ */

/* $OpenBSD: ypv1.h,v 1.1 1997/03/30 20:51:24 maja Exp $ */

/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _YPV1_H_RPCGEN
#define _YPV1_H_RPCGEN

#include <rpc/rpc.h>

#define YPOLDVERS ((u_long)1)

enum ypreqtype {
	YPREQ_KEY = 1,
	YPREQ_NOKEY = 2,
	YPREQ_MAP_PARMS = 3
};
typedef enum ypreqtype ypreqtype;
#ifdef __cplusplus 
extern "C" bool_t xdr_ypreqtype(XDR *, ypreqtype*);
#elif __STDC__ 
extern  bool_t xdr_ypreqtype(XDR *, ypreqtype*);
#else /* Old Style C */ 
bool_t xdr_ypreqtype();
#endif /* Old Style C */ 

typedef struct {
	ypreqtype yp_reqtype;
	union {
		struct ypreq_key yp_req_keytype;
		struct ypreq_nokey yp_req_nokeytype;
		struct ypmap_parms yp_req_map_parmstype;
	}yp_reqbody;
} yprequest;
#ifdef __cplusplus 
extern "C" bool_t xdr_yprequest(XDR *, yprequest*);
#elif __STDC__ 
extern  bool_t xdr_yprequest(XDR *, yprequest*);
#else /* Old Style C */ 
bool_t xdr_yprequest();
#endif /* Old Style C */ 

#define YPMATCH_REQTYPE YPREQ_KEY
#define ypmatch_req_domain yp_reqbody.yp_req_keytype.domain
#define ypmatch_req_map yp_reqbody.yp_req_keytype.map
#define ypmatch_req_keydat yp_reqbody.yp_req_keytype.key
#define ypmatch_req_keyptr yp_reqbody.yp_req_keytype.key.keydat_val
#define ypmatch_req_keysize yp_reqbody.yp_req_keytype.key.keydat_len

#define YPFIRST_REQTYPE YPREQ_NOKEY
#define ypfirst_req_domain yp_reqbody.yp_req_nokeytype.domain
#define ypfirst_req_map yp_reqbody.yp_req_nokeytype.map

#define YPNEXT_REQTYPE YPREQ_KEY
#define ypnext_req_domain yp_reqbody.yp_req_keytype.domain
#define ypnext_req_map yp_reqbody.yp_req_keytype.map
#define ypnext_req_keydat yp_reqbody.yp_req_keytype.key
#define ypnext_req_keyptr yp_reqbody.yp_req_keytype.key.keydat_val
#define ypnext_req_keysize yp_reqbody.yp_req_keytype.key.keydat_len

#define YPPUSH_REQTYPE YPREQ_NOKEY
#define yppush_req_domain yp_reqbody.yp_req_nokeytype.domain
#define yppush_req_map yp_reqbody.yp_req_nokeytype.map

#define YPPULL_REQTYPE YPREQ_NOKEY
#define yppull_req_domain yp_reqbody.yp_req_nokeytype.domain
#define yppull_req_map yp_reqbody.yp_req_nokeytype.map

#define YPPOLL_REQTYPE YPREQ_NOKEY
#define yppoll_req_domain yp_reqbody.yp_req_nokeytype.domain
#define yppoll_req_map yp_reqbody.yp_req_nokeytype.map

#define YPGET_REQTYPE YPREQ_MAP_PARMS
#define ypget_req_domain yp_reqbody.yp_req_map_parmstype.domain
#define ypget_req_map yp_reqbody.yp_req_map_parmstype.map
#define ypget_req_ordernum yp_reqbody.yp_req_map_parmstype.ordernum
#define ypget_req_owner yp_reqbody.yp_req_map_parmstype.peer

enum ypresptype {
	YPRESP_VAL = 1,
	YPRESP_KEY_VAL = 2,
	YPRESP_MAP_PARMS = 3
};
typedef enum ypresptype ypresptype;
#ifdef __cplusplus 
extern "C" bool_t xdr_ypresptype(XDR *, ypresptype*);
#elif __STDC__ 
extern  bool_t xdr_ypresptype(XDR *, ypresptype*);
#else /* Old Style C */ 
bool_t xdr_ypresptype();
#endif /* Old Style C */ 

typedef struct {
	ypresptype yp_resptype;
	union {
		struct ypresp_val yp_resp_valtype;
		struct ypresp_key_val yp_resp_key_valtype;
		struct ypmap_parms yp_resp_map_parmstype;
	} yp_respbody;
} ypresponse;
#ifdef __cplusplus 
extern "C" bool_t xdr_ypresponse(XDR *, ypresponse*);
#elif __STDC__ 
extern  bool_t xdr_ypresponse(XDR *, ypresponse*);
#else /* Old Style C */ 
bool_t xdr_ypresponse();
#endif /* Old Style C */ 

#define YPMATCH_RESPTYPE YPRESP_VAL
#define ypmatch_resp_status yp_respbody.yp_resp_valtype.stat
#define ypmatch_resp_val yp_respbody.yp_resp_valtype
#define ypmatch_resp_valdat yp_respbody.yp_resp_valtype.val
#define ypmatch_resp_valptr yp_respbody.yp_resp_valtype.val.valdat_val
#define ypmatch_resp_valsize yp_respbody.yp_resp_valtype.val.valdat_len

#define YPFIRST_RESPTYPE YPRESP_KEY_VAL
#define ypfirst_resp_status yp_respbody.yp_resp_key_valtype.stat
#define ypfirst_resp_keydat yp_respbody.yp_resp_key_valtype.key
#define ypfirst_resp_keyptr yp_respbody.yp_resp_key_valtype.key.keydat_val
#define ypfirst_resp_keysize yp_respbody.yp_resp_key_valtype.key.keydat_len
#define ypfirst_resp_val yp_respbody.yp_resp_key_valtype
#define ypfirst_resp_valdat yp_respbody.yp_resp_key_valtype.val
#define ypfirst_resp_valptr yp_respbody.yp_resp_key_valtype.val.valdat_val
#define ypfirst_resp_valsize yp_respbody.yp_resp_key_valtype.val.valdat_len

#define YPNEXT_RESPTYPE YPRESP_KEY_VAL
#define ypnext_resp_status yp_respbody.yp_resp_key_valtype.stat
#define ypnext_resp_keydat yp_respbody.yp_resp_key_valtype.key
#define ypnext_resp_keyptr yp_respbody.yp_resp_key_valtype.key.keydat_val
#define ypnext_resp_keysize yp_respbody.yp_resp_key_valtype.key.keydat_len
#define ypnext_resp_val yp_respbody.yp_resp_key_valtype
#define ypnext_resp_valdat yp_respbody.yp_resp_key_valtype.val
#define ypnext_resp_valptr yp_respbody.yp_resp_key_valtype.val.valdat_val
#define ypnext_resp_valsize yp_respbody.yp_resp_key_valtype.val.valdat_len

#define YPPOLL_RESPTYPE YPRESP_MAP_PARMS
#define yppoll_resp_domain yp_respbody.yp_resp_map_parmstype.domain
#define yppoll_resp_map yp_respbody.yp_resp_map_parmstype.map
#define yppoll_resp_ordernum yp_respbody.yp_resp_map_parmstype.ordernum
#define yppoll_resp_owner yp_respbody.yp_resp_map_parmstype.peer

#ifdef __cplusplus
#define YPOLDPROC_NULL ((u_long)0)
extern "C" void * ypproc_null_1(void *, CLIENT *);
extern "C" void * ypproc_null_1_svc(void *, struct svc_req *);
#define YPOLDPROC_DOMAIN ((u_long)1)
extern "C" bool_t * ypproc_domain_1(domainname *, CLIENT *);
extern "C" bool_t * ypproc_domain_1_svc(domainname *, struct svc_req *);
#define YPOLDPROC_DOMAIN_NONACK ((u_long)2)
extern "C" bool_t * ypproc_domain_nonack_1(domainname *, CLIENT *);
extern "C" bool_t * ypproc_domain_nonack_1_svc(domainname *, struct svc_req *);
#define YPOLDPROC_MATCH ((u_long)3)
extern "C" ypresponse * ypproc_match_1(yprequest *, CLIENT *);
extern "C" ypresponse * ypproc_match_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_FIRST ((u_long)4)
extern "C" ypresponse * ypproc_first_1(yprequest *, CLIENT *);
extern "C" ypresponse * ypproc_first_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_NEXT ((u_long)5)
extern "C" ypresponse * ypproc_next_1(yprequest *, CLIENT *);
extern "C" ypresponse * ypproc_next_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_POLL ((u_long)6)
extern "C" ypresponse * ypproc_poll_1(yprequest *, CLIENT *);
extern "C" ypresponse * ypproc_poll_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_PUSH ((u_long)7)
extern "C" void * ypproc_push_1(yprequest *, CLIENT *);
extern "C" void * ypproc_push_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_PULL ((u_long)8)
extern "C" void * ypproc_pull_1(yprequest *, CLIENT *);
extern "C" void * ypproc_pull_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_GET ((u_long)9)
extern "C" void * ypproc_get_1(yprequest *, CLIENT *);
extern "C" void * ypproc_get_1_svc(yprequest *, struct svc_req *);

#elif __STDC__
#define YPOLDPROC_NULL ((u_long)0)
extern  void * ypproc_null_1(void *, CLIENT *);
extern  void * ypproc_null_1_svc(void *, struct svc_req *);
#define YPOLDPROC_DOMAIN ((u_long)1)
extern  bool_t * ypproc_domain_1(domainname *, CLIENT *);
extern  bool_t * ypproc_domain_1_svc(domainname *, struct svc_req *);
#define YPOLDPROC_DOMAIN_NONACK ((u_long)2)
extern  bool_t * ypproc_domain_nonack_1(domainname *, CLIENT *);
extern  bool_t * ypproc_domain_nonack_1_svc(domainname *, struct svc_req *);
#define YPOLDPROC_MATCH ((u_long)3)
extern  ypresponse * ypproc_match_1(yprequest *, CLIENT *);
extern  ypresponse * ypproc_match_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_FIRST ((u_long)4)
extern  ypresponse * ypproc_first_1(yprequest *, CLIENT *);
extern  ypresponse * ypproc_first_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_NEXT ((u_long)5)
extern  ypresponse * ypproc_next_1(yprequest *, CLIENT *);
extern  ypresponse * ypproc_next_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_POLL ((u_long)6)
extern  ypresponse * ypproc_poll_1(yprequest *, CLIENT *);
extern  ypresponse * ypproc_poll_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_PUSH ((u_long)7)
extern  void * ypproc_push_1(yprequest *, CLIENT *);
extern  void * ypproc_push_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_PULL ((u_long)8)
extern  void * ypproc_pull_1(yprequest *, CLIENT *);
extern  void * ypproc_pull_1_svc(yprequest *, struct svc_req *);
#define YPOLDPROC_GET ((u_long)9)
extern  void * ypproc_get_1(yprequest *, CLIENT *);
extern  void * ypproc_get_1_svc(yprequest *, struct svc_req *);

#else /* Old Style C */ 
#define YPOLDPROC_NULL ((u_long)0)
extern  void * ypproc_null_1();
extern  void * ypproc_null_1_svc();
#define YPOLDPROC_DOMAIN ((u_long)1)
extern  bool_t * ypproc_domain_1();
extern  bool_t * ypproc_domain_1_svc();
#define YPOLDPROC_DOMAIN_NONACK ((u_long)2)
extern  bool_t * ypproc_domain_nonack_1();
extern  bool_t * ypproc_domain_nonack_1_svc();
#define YPOLDPROC_MATCH ((u_long)3)
extern  ypresponse * ypproc_match_1();
extern  ypresponse * ypproc_match_1_svc();
#define YPOLDPROC_FIRST ((u_long)4)
extern  ypresponse * ypproc_first_1();
extern  ypresponse * ypproc_first_1_svc();
#define YPOLDPROC_NEXT ((u_long)5)
extern  ypresponse * ypproc_next_1();
extern  ypresponse * ypproc_next_1_svc();
#define YPOLDPROC_POLL ((u_long)6)
extern  ypresponse * ypproc_poll_1();
extern  ypresponse * ypproc_poll_1_svc();
#define YPOLDPROC_PUSH ((u_long)7)
extern  void * ypproc_push_1();
extern  void * ypproc_push_1_svc();
#define YPOLDPROC_PULL ((u_long)8)
extern  void * ypproc_pull_1();
extern  void * ypproc_pull_1_svc();
#define YPOLDPROC_GET ((u_long)9)
extern  void * ypproc_get_1();
extern  void * ypproc_get_1_svc();
#endif /* Old Style C */ 

#endif /* !_YPV1_H_RPCGEN */

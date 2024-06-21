#include "pch.h"
#include "ps_codec.h"

static const char* THIS_FILE = "PSCODEC";

static const pj_str_t ID_VIDEO = { (char*)"video", 5 };
static const pj_str_t ID_IN = { (char*)"IN", 2 };
static const pj_str_t ID_IP4 = { (char*)"IP4", 3 };
static const pj_str_t ID_IP6 = { (char*)"IP6", 3 };
static const pj_str_t ID_RTPMAP = { (char*)"rtpmap", 6 };
static char poolBuf[512] = { NULL };

PJ_DEF(pj_status_t) pjmedia_vid_codec_mgr_find_codecs_by_id2(
	pjmedia_vid_codec_mgr *mgr,
	const pj_str_t *codec_id,
	unsigned *count,
	const pjmedia_vid_codec_info *p_info[],
	unsigned prio[]);

static pj_status_t ps_test_alloc(pjmedia_vid_codec_factory *factory, const pjmedia_vid_codec_info *info);
static pj_status_t ps_default_attr(pjmedia_vid_codec_factory *factory, const pjmedia_vid_codec_info *info, pjmedia_vid_codec_param *attr);
static pj_status_t ps_enum_info(pjmedia_vid_codec_factory *factory,
	unsigned *count,
	pjmedia_vid_codec_info info[]);
static pj_status_t ps_alloc_codec(pjmedia_vid_codec_factory *factory,
	const pjmedia_vid_codec_info *info,
	pjmedia_vid_codec **p_codec);
static pj_status_t ps_dealloc_codec(pjmedia_vid_codec_factory *factory,
	pjmedia_vid_codec *codec);

static pjmedia_vid_codec_factory_op ps_factory_op =
{
	&ps_test_alloc,
	&ps_default_attr,
	&ps_enum_info,
	&ps_alloc_codec,
	&ps_dealloc_codec
};

static struct ps_factory_t
{
	pjmedia_vid_codec_factory base;
	pjmedia_vid_codec_mgr     *mgr;
	pj_pool_factory           *pf;
	pj_pool_t                 *pool;
} ps_factory;


static pj_status_t ps_test_alloc(pjmedia_vid_codec_factory *factory, const pjmedia_vid_codec_info *info)
{
	PJ_ASSERT_RETURN(factory == &ps_factory.base, PJ_EINVAL);

	if (info->fmt_id == PJMEDIA_FORMAT_PS && info->pt != 0)
	{
		return PJ_SUCCESS;
	}

	return PJMEDIA_CODEC_EUNSUP;
}

static pj_status_t ps_default_attr(pjmedia_vid_codec_factory *factory, const pjmedia_vid_codec_info *info, pjmedia_vid_codec_param *attr)
{
	return PJ_SUCCESS;
}

static pj_status_t ps_enum_info(pjmedia_vid_codec_factory *factory,
	unsigned *count,
	pjmedia_vid_codec_info info[])
{
	PJ_ASSERT_RETURN(info && *count > 0, PJ_EINVAL);
	PJ_ASSERT_RETURN(factory == &ps_factory.base, PJ_EINVAL);

	*count = 1;
	info->fmt_id = (pjmedia_format_id)PJMEDIA_FORMAT_PS;
	info->pt = PJMEDIA_RTP_PT_PS;
	info->encoding_name = pj_str((char*)"PS");
	info->encoding_desc = pj_str((char*)"PS codec");
	info->clock_rate = 90000;
	info->dir = PJMEDIA_DIR_ENCODING_DECODING;
	info->dec_fmt_id_cnt = 1;
	info->dec_fmt_id[0] = (pjmedia_format_id)PJMEDIA_FORMAT_PS;
	info->packings = PJMEDIA_VID_PACKING_PACKETS | PJMEDIA_VID_PACKING_WHOLE;
	info->fps_cnt = 3;
	info->fps[0].num = 15;
	info->fps[0].denum = 1;
	info->fps[1].num = 25;
	info->fps[1].denum = 1;
	info->fps[2].num = 30;
	info->fps[2].denum = 1;

	return PJ_SUCCESS;

}



static pj_status_t ps_alloc_codec(pjmedia_vid_codec_factory *factory,
	const pjmedia_vid_codec_info *info,
	pjmedia_vid_codec **p_codec)
{
	*p_codec = NULL;
	return PJ_SUCCESS;
}

static pj_status_t ps_dealloc_codec(pjmedia_vid_codec_factory *factory,
	pjmedia_vid_codec *codec)
{
	return PJ_SUCCESS;
}


PJ_DEF(pj_status_t) pjmedia_codec_ps_vid_init(pjmedia_vid_codec_mgr *mgr, pj_pool_factory *pf)
{
	const pj_str_t ps_name = { (char*)"PS", 2 };
	pj_status_t status;

	if (ps_factory.pool != NULL) {
		/* Already initialized. */
		return PJ_SUCCESS;
	}

	if (!mgr) mgr = pjmedia_vid_codec_mgr_instance();
	PJ_ASSERT_RETURN(mgr, PJ_EINVAL);

	/* Create OpenH264 codec factory. */
	ps_factory.base.op = &ps_factory_op;
	ps_factory.base.factory_data = NULL;
	ps_factory.mgr = mgr;
	ps_factory.pf = pf;
	ps_factory.pool = pj_pool_create_on_buf("psCodec", poolBuf, sizeof(poolBuf));
	if (!ps_factory.pool)
		return PJ_ENOMEM;


	/* Register codec factory to codec manager. */
	status = pjmedia_vid_codec_mgr_register_factory(mgr, &ps_factory.base);
	if (status != PJ_SUCCESS)
		goto on_error;

	SIP_LOG(SIP_INFO, (THIS_FILE, "PS codec initialized"));

	/* Done. */
	return PJ_SUCCESS;

on_error:
	pj_pool_release(ps_factory.pool);
	ps_factory.pool = NULL;
	return status;
}

/*
* Unregister ps codecs factory from pjmedia endpoint.
*/
PJ_DEF(pj_status_t) pjmedia_codec_ps_vid_deinit(void)
{
	pj_status_t status = PJ_SUCCESS;

	if (ps_factory.pool == NULL) {
		/* Already deinitialized */
		return PJ_SUCCESS;
	}

	/* Unregister OpenH264 codecs factory. */
	status = pjmedia_vid_codec_mgr_unregister_factory(ps_factory.mgr, &ps_factory.base);

	/* Destroy pool. */
	//pj_pool_release(ps_factory.pool);
	ps_factory.pool = NULL;

	return status;
}


pj_status_t pjmedia_vid_stream_info_from_sdp2(
	pjmedia_vid_stream_info *si,
	pj_pool_t *pool,
	pjmedia_endpt *endpt,
	const pjmedia_sdp_session *local,
	const pjmedia_sdp_session *remote,
	unsigned stream_idx)
{
	const pj_str_t STR_INACTIVE = { (char*)"inactive", 8 };
	const pj_str_t STR_SENDONLY = { (char*)"sendonly", 8 };
	const pj_str_t STR_RECVONLY = { (char*)"recvonly", 8 };

	const pjmedia_sdp_attr *attr;
	const pjmedia_sdp_media *local_m;
	const pjmedia_sdp_media *rem_m;
	const pjmedia_sdp_conn *local_conn;
	const pjmedia_sdp_conn *rem_conn;
	int rem_af, local_af;
	pj_sockaddr local_addr;
	unsigned i;
	pj_status_t status;

	PJ_UNUSED_ARG(endpt);

	/* Validate arguments: */
	PJ_ASSERT_RETURN(pool && si && local && remote, PJ_EINVAL);
	PJ_ASSERT_RETURN(stream_idx < local->media_count, PJ_EINVAL);
	PJ_ASSERT_RETURN(stream_idx < remote->media_count, PJ_EINVAL);

	/* Keep SDP shortcuts */
	local_m = local->media[stream_idx];
	rem_m = remote->media[stream_idx];

	local_conn = local_m->conn ? local_m->conn : local->conn;
	if (local_conn == NULL)
		return PJMEDIA_SDP_EMISSINGCONN;

	rem_conn = rem_m->conn ? rem_m->conn : remote->conn;
	if (rem_conn == NULL)
		return PJMEDIA_SDP_EMISSINGCONN;

	/* Media type must be video */
	if (pj_stricmp(&local_m->desc.media, &ID_VIDEO) != 0)
		return PJMEDIA_EINVALIMEDIATYPE;


	/* Reset: */

	pj_bzero(si, sizeof(*si));

	/* Media type: */
	si->type = PJMEDIA_TYPE_VIDEO;

	/* Transport protocol */

	/* At this point, transport type must be compatible,
	* the transport instance will do more validation later.
	*/
	status = pjmedia_sdp_transport_cmp(&rem_m->desc.transport,
		&local_m->desc.transport);
	if (status != PJ_SUCCESS)
		return PJMEDIA_SDPNEG_EINVANSTP;

	/* Get the transport protocol */
	si->proto = (pjmedia_tp_proto)pjmedia_sdp_transport_get_proto(&local_m->desc.transport);

	/* Return success if transport protocol is not RTP/AVP compatible */
	if (!PJMEDIA_TP_PROTO_HAS_FLAG(si->proto, PJMEDIA_TP_PROTO_RTP_AVP))
		return PJ_SUCCESS;


	/* Check address family in remote SDP */
	rem_af = pj_AF_UNSPEC();
	if (pj_stricmp(&rem_conn->net_type, &ID_IN) == 0) {
		if (pj_stricmp(&rem_conn->addr_type, &ID_IP4) == 0) {
			rem_af = pj_AF_INET();
		}
		else if (pj_stricmp(&rem_conn->addr_type, &ID_IP6) == 0) {
			rem_af = pj_AF_INET6();
		}
	}

	if (rem_af == pj_AF_UNSPEC()) {
		/* Unsupported address family */
		return PJ_EAFNOTSUP;
	}

	/* Set remote address: */
	status = pj_sockaddr_init(rem_af, &si->rem_addr, &rem_conn->addr,
		rem_m->desc.port);
	if (status != PJ_SUCCESS) {
		/* Invalid IP address. */
		return PJMEDIA_EINVALIDIP;
	}

	/* Check address family of local info */
	local_af = pj_AF_UNSPEC();
	if (pj_stricmp(&local_conn->net_type, &ID_IN) == 0) {
		if (pj_stricmp(&local_conn->addr_type, &ID_IP4) == 0) {
			local_af = pj_AF_INET();
		}
		else if (pj_stricmp(&local_conn->addr_type, &ID_IP6) == 0) {
			local_af = pj_AF_INET6();
		}
	}

	if (local_af == pj_AF_UNSPEC()) {
		/* Unsupported address family */
		return PJ_SUCCESS;
	}

	/* Set remote address: */
	status = pj_sockaddr_init(local_af, &local_addr, &local_conn->addr,
		local_m->desc.port);
	if (status != PJ_SUCCESS) {
		/* Invalid IP address. */
		return PJMEDIA_EINVALIDIP;
	}

	/* Local and remote address family must match, except when ICE is used
	* by both sides (see also ticket #1952).
	*/
	if (local_af != rem_af) {
		const pj_str_t STR_ICE_CAND = { (char*)"candidate", 9 };
		if (pjmedia_sdp_media_find_attr(rem_m, &STR_ICE_CAND, NULL) == NULL ||
			pjmedia_sdp_media_find_attr(local_m, &STR_ICE_CAND, NULL) == NULL)
		{
			return PJ_EAFNOTSUP;
		}
	}

	/* Media direction: */

	if (local_m->desc.port == 0 ||
		pj_sockaddr_has_addr(&local_addr) == PJ_FALSE ||
		pj_sockaddr_has_addr(&si->rem_addr) == PJ_FALSE ||
		pjmedia_sdp_media_find_attr(local_m, &STR_INACTIVE, NULL) != NULL)
	{
		/* Inactive stream. */

		si->dir = PJMEDIA_DIR_NONE;

	}
	else if (pjmedia_sdp_media_find_attr(local_m, &STR_SENDONLY, NULL) != NULL) {

		/* Send only stream. */

		si->dir = PJMEDIA_DIR_ENCODING;

	}
	else if (pjmedia_sdp_media_find_attr(local_m, &STR_RECVONLY, NULL) != NULL) {

		/* Recv only stream. */

		si->dir = PJMEDIA_DIR_DECODING;

	}
	else {

		/* Send and receive stream. */

		si->dir = PJMEDIA_DIR_ENCODING_DECODING;

	}

	/* No need to do anything else if stream is rejected */
	if (local_m->desc.port == 0) {
		return PJ_SUCCESS;
	}

	/* Check if "rtcp-mux" is present in the SDP. */
	attr = pjmedia_sdp_attr_find2(rem_m->attr_count, rem_m->attr,
		"rtcp-mux", NULL);
	if (attr)
		si->rtcp_mux = PJ_TRUE;

	/* If "rtcp" attribute is present in the SDP, set the RTCP address
	* from that attribute. Otherwise, calculate from RTP address.
	*/
	attr = pjmedia_sdp_attr_find2(rem_m->attr_count, rem_m->attr,
		"rtcp", NULL);
	if (attr) {
		pjmedia_sdp_rtcp_attr rtcp;
		status = pjmedia_sdp_attr_get_rtcp(attr, &rtcp);
		if (status == PJ_SUCCESS) {
			if (rtcp.addr.slen) {
				status = pj_sockaddr_init(rem_af, &si->rem_rtcp, &rtcp.addr,
					(pj_uint16_t)rtcp.port);
			}
			else {
				pj_sockaddr_init(rem_af, &si->rem_rtcp, NULL,
					(pj_uint16_t)rtcp.port);
				pj_memcpy(pj_sockaddr_get_addr(&si->rem_rtcp),
					pj_sockaddr_get_addr(&si->rem_addr),
					pj_sockaddr_get_addr_len(&si->rem_addr));
			}
		}
	}

	if (!pj_sockaddr_has_addr(&si->rem_rtcp)) {
		int rtcp_port;

		pj_memcpy(&si->rem_rtcp, &si->rem_addr, sizeof(pj_sockaddr));
		rtcp_port = pj_sockaddr_get_port(&si->rem_addr) + 1;
		pj_sockaddr_set_port(&si->rem_rtcp, (pj_uint16_t)rtcp_port);
	}

	/* Check if "ssrc" attribute is present in the SDP. */
	for (i = 0; i < rem_m->attr_count; i++) {
		if (pj_strcmp2(&rem_m->attr[i]->name, "ssrc") == 0) {
			pjmedia_sdp_ssrc_attr ssrc;

			status = pjmedia_sdp_attr_get_ssrc(
				(const pjmedia_sdp_attr *)rem_m->attr[i], &ssrc);
			if (status == PJ_SUCCESS) {
				si->has_rem_ssrc = PJ_TRUE;
				si->rem_ssrc = ssrc.ssrc;
				if (ssrc.cname.slen > 0) {
					pj_strdup(pool, &si->rem_cname, &ssrc.cname);
					break;
				}
			}
		}
	}

	/* Get codec info and param */
	//status = get_video_codec_info_param2(si, pool, NULL, local_m, rem_m);

	/* Leave SSRC to random. */
	si->ssrc = pj_rand();

	/* Set default jitter buffer parameter. */
	si->jb_init = si->jb_max = si->jb_min_pre = si->jb_max_pre = -1;

	return status;
}


static pj_status_t get_video_codec_info_param2(pjmedia_vid_stream_info *si,
	pj_pool_t *pool,
	pjmedia_vid_codec_mgr *mgr,
	const pjmedia_sdp_media *local_m,
	const pjmedia_sdp_media *rem_m)
{
	unsigned pt = 0;
	const pjmedia_vid_codec_info *p_info;
	pj_status_t status;

	pt = pj_strtoul(&local_m->desc.fmt[0]);

	/* Get payload type for receiving direction */
	si->rx_pt = pt;

	const pjmedia_sdp_attr *attr;
	pjmedia_sdp_rtpmap *rtpmap;
	pjmedia_codec_id codec_id;
	pj_str_t codec_id_st;
	unsigned i;

	/* Determine payload type for outgoing channel, by finding
	* dynamic payload type in remote SDP that matches the answer.
	*/
	si->tx_pt = 0xFFFF;
	for (i = 0; i<rem_m->desc.fmt_count; ++i) {
		if (pjmedia_sdp_neg_fmt_match(NULL,
			(pjmedia_sdp_media*)local_m, 0,
			(pjmedia_sdp_media*)rem_m, i, 0) ==
			PJ_SUCCESS)
		{
			/* Found matched codec. */
			si->tx_pt = pj_strtoul(&rem_m->desc.fmt[i]);
			break;
		}
	}

	if (si->tx_pt == 0xFFFF)
		return PJMEDIA_EMISSINGRTPMAP;

	/* For dynamic payload types, get codec name from the rtpmap */
	attr = pjmedia_sdp_media_find_attr(local_m, &ID_RTPMAP,
		&local_m->desc.fmt[0]);
	if (attr == NULL)
		return PJMEDIA_EMISSINGRTPMAP;

	status = pjmedia_sdp_attr_to_rtpmap(pool, attr, &rtpmap);
	if (status != PJ_SUCCESS)
		return status;

	/* Then get the codec info from the codec manager */
	pj_ansi_snprintf(codec_id, sizeof(codec_id), "%.*s/",
		(int)rtpmap->enc_name.slen, rtpmap->enc_name.ptr);
	codec_id_st = pj_str(codec_id);
	i = 1;
	status = pjmedia_vid_codec_mgr_find_codecs_by_id2(mgr, &codec_id_st,
		&i, &p_info, NULL);
	if (status != PJ_SUCCESS)
		return status;

	si->codec_info = *p_info;


	/* Request for codec with the correct packing for streaming */
	si->codec_info.packings = PJMEDIA_VID_PACKING_PACKETS;

	/* Now that we have codec info, get the codec param. */
	si->codec_param = PJ_POOL_ALLOC_T(pool, pjmedia_vid_codec_param);
	status = pjmedia_vid_codec_mgr_get_default_param(mgr,
		&si->codec_info,
		si->codec_param);

	/* Adjust encoding bitrate, if higher than remote preference. The remote
	* bitrate preference is read from SDP "b=TIAS" line in media level.
	*/
	if ((si->dir & PJMEDIA_DIR_ENCODING) && rem_m->bandw_count) {
		unsigned i, bandw = 0;

		for (i = 0; i < rem_m->bandw_count; ++i) {
			const pj_str_t STR_BANDW_MODIFIER_TIAS = { (char*)"TIAS", 4 };
			if (!pj_stricmp(&rem_m->bandw[i]->modifier,
				&STR_BANDW_MODIFIER_TIAS))
			{
				bandw = rem_m->bandw[i]->value;
				break;
			}
		}

		if (bandw) {
			pjmedia_video_format_detail *enc_vfd;
			enc_vfd = pjmedia_format_get_video_format_detail(
				&si->codec_param->enc_fmt, PJ_TRUE);
			if (!enc_vfd->avg_bps || enc_vfd->avg_bps > bandw)
				enc_vfd->avg_bps = bandw * 3 / 4;
			if (!enc_vfd->max_bps || enc_vfd->max_bps > bandw)
				enc_vfd->max_bps = bandw;
		}
	}

	/* Get remote fmtp for our encoder. */
	pjmedia_stream_info_parse_fmtp(pool, rem_m, si->tx_pt,
		&si->codec_param->enc_fmtp);

	/* Get local fmtp for our decoder. */
	pjmedia_stream_info_parse_fmtp(pool, local_m, si->rx_pt,
		&si->codec_param->dec_fmtp);

	/* When direction is NONE (it means SDP negotiation has failed) we don't
	* need to return a failure here, as returning failure will cause
	* the whole SDP to be rejected. See ticket #:
	*	http://
	*
	* Thanks Alain Totouom
	*/
	if (status != PJ_SUCCESS && si->dir != PJMEDIA_DIR_NONE)
		return status;

	return PJ_SUCCESS;
}


PJ_DEF(pj_status_t) pjmedia_vid_codec_mgr_find_codecs_by_id2(
	pjmedia_vid_codec_mgr *mgr,
	const pj_str_t *codec_id,
	unsigned *count,
	const pjmedia_vid_codec_info *p_info[],
	unsigned prio[])
{
	unsigned found = 0;

	PJ_ASSERT_RETURN(codec_id && count && *count, PJ_EINVAL);

	if (!mgr) mgr = pjmedia_vid_codec_mgr_instance();
	PJ_ASSERT_RETURN(mgr, PJ_EINVAL);

	if (codec_id->slen == 0 || pj_strnicmp2(codec_id, (char*)"PS", codec_id->slen) == 0) {
		found = 1;
	}

	return found ? PJ_SUCCESS : PJ_ENOTFOUND;
}

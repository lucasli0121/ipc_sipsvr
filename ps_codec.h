#pragma once
#include "pch.h"

#define PJMEDIA_FORMAT_PS  PJMEDIA_FORMAT_PACK('P', 'S', ' ', ' ')
#define PJMEDIA_RTP_PT_PS 97

pj_status_t pjmedia_vid_stream_info_from_sdp2(
	pjmedia_vid_stream_info *si,
	pj_pool_t *pool,
	pjmedia_endpt *endpt,
	const pjmedia_sdp_session *local,
	const pjmedia_sdp_session *remote,
	unsigned stream_idx);

PJ_DEF(pj_status_t) pjmedia_codec_ps_vid_init(pjmedia_vid_codec_mgr *mgr, pj_pool_factory *pf);
PJ_DEF(pj_status_t) pjmedia_codec_ps_vid_deinit(void);
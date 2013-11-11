// PJSIPDll.cpp : Defines the exported functions for the DLL application.
//

#include "PJSIPDll.h"
#include <memory.h>
#include <iostream>
#include <functional>

#ifndef PJSUA_HAS_VIDEO
#define PJSUA_HAS_VIDEO 0
#endif

#define THIS_FILE	"PJSIPDll.cpp"
#define NO_LIMIT	(int)0x7FFFFFFF

//static DumpPJLOG* on_pj_write_cb = 0;
//static EventLOG* on_event_cb = 0;

CPJSIPDll *pjsipDll = NULL;
extern void Dialer_Confirmed();


static void pj_dll_log_write(int level, const char *buffer, int len)
{
#if 0
	if(pjsipDll && (pjsipDll->isAppExiting() == PJ_FALSE) && (on_pj_write_cb)) (on_pj_write_cb)(level, buffer, len);
#else
    std::cout << buffer << std::endl;   
#endif
}

static void pj_dll_event_write(int level, const char *buffer, int len)
{
#if 0
	if(pjsipDll && (pjsipDll->isAppExiting() == PJ_FALSE) && (on_event_cb)) (on_event_cb)(level, buffer, len);
#else
    std::cout << buffer << std::endl;   
#endif
}


/* Set default config. */
void CPJSIPDll::default_config()
{
    char tmp[80];
    unsigned i;
	struct app_config_s *cfg = &app_config;

    pjsua_config_default(&cfg->cfg);
    pj_ansi_sprintf(tmp, "PJSUA v%s %s", pj_get_version(),
		    pj_get_sys_info()->info.ptr);
    pj_strdup2_with_null(app_config.pool, &cfg->cfg.user_agent, tmp);

    pjsua_logging_config_default(&cfg->log_cfg);
    pjsua_media_config_default(&cfg->media_cfg);
    pjsua_transport_config_default(&cfg->udp_cfg);
    cfg->udp_cfg.port = 5070;
	
    pjsua_transport_config_default(&cfg->rtp_cfg);
    cfg->rtp_cfg.port = 4000;
    cfg->redir_op = PJSIP_REDIRECT_ACCEPT_REPLACE;
    cfg->duration = NO_LIMIT;
    cfg->wav_id = PJSUA_INVALID_ID;
    cfg->rec_id = PJSUA_INVALID_ID;
    cfg->wav_port = PJSUA_INVALID_ID;
    cfg->rec_port = PJSUA_INVALID_ID;
    cfg->mic_level = cfg->speaker_level = 1.0;
	#if 1
    cfg->capture_dev = PJSUA_INVALID_ID;
    cfg->playback_dev = PJSUA_INVALID_ID;
	#else
    cfg->capture_dev = 3;
    cfg->playback_dev = 2;
	#endif
    cfg->capture_lat = PJMEDIA_SND_DEFAULT_REC_LATENCY;
    cfg->playback_lat = PJMEDIA_SND_DEFAULT_PLAY_LATENCY;
    cfg->ringback_slot = PJSUA_INVALID_ID;
    cfg->ring_slot = PJSUA_INVALID_ID;

    for (i=0; i<PJ_ARRAY_SIZE(cfg->acc_cfg); ++i)
		pjsua_acc_config_default(&cfg->acc_cfg[i]);

	cfg->log_cfg.log_filename = pj_str((char *)"PJSIPDll.log");

    for (i=0; i<PJ_ARRAY_SIZE(cfg->buddy_cfg); ++i)
	pjsua_buddy_config_default(&cfg->buddy_cfg[i]);
	cfg->vid.vcapture_dev = PJMEDIA_VID_DEFAULT_CAPTURE_DEV;
    cfg->vid.vrender_dev = PJMEDIA_VID_DEFAULT_RENDER_DEV;
    cfg->aud_cnt = 1;

    cfg->avi_def_idx = PJSUA_INVALID_ID;
	//cfg->no_udp = true;
	//cfg->media_cfg.clock_rate = 44100;
}



/*
 * Print log of call states. Since call states may be too long for logger,
 * printing it is a bit tricky, it should be printed part by part as long 
 * as the logger can accept.
 */
void CPJSIPDll::log_call_dump(int call_id) 
{
    unsigned call_dump_len;
    unsigned part_len;
    unsigned part_idx;
    unsigned log_decor;

    pjsua_call_dump(call_id, PJ_TRUE, some_buf, 
		    sizeof(some_buf), "  ");
    call_dump_len = strlen(some_buf);

    log_decor = pj_log_get_decor();
    pj_log_set_decor(log_decor & ~(PJ_LOG_HAS_NEWLINE | PJ_LOG_HAS_CR));
    PJ_LOG(3,(THIS_FILE, "\n"));
    pj_log_set_decor(0);

    part_idx = 0;
    part_len = PJ_LOG_MAX_SIZE-80;
    while (part_idx < call_dump_len) {
	char p_orig, *p;

	p = &some_buf[part_idx];
	if (part_idx + part_len > call_dump_len)
	    part_len = call_dump_len - part_idx;
	p_orig = p[part_len];
	p[part_len] = '\0';
	PJ_LOG(3,(THIS_FILE, "%s", p));
	p[part_len] = p_orig;
	part_idx += part_len;
    }
    pj_log_set_decor(log_decor);
}

/*****************************************************************************
 * Console application
 */

void CPJSIPDll::ringback_start(pjsua_call_id call_id)
{
    if (app_config.no_tones)
		return;

    if (app_config.call_data[call_id].ringback_on)
		return;

    app_config.call_data[call_id].ringback_on = PJ_TRUE;

    if (++app_config.ringback_cnt==1 && 
		app_config.ringback_slot!=PJSUA_INVALID_ID) 
    {
		pjsua_conf_connect(app_config.ringback_slot, 0);
    }
}

void CPJSIPDll::ring_stop(pjsua_call_id call_id)
{
    if (app_config.no_tones)
		return;

    if (app_config.call_data[call_id].ringback_on) {
		app_config.call_data[call_id].ringback_on = PJ_FALSE;

		pj_assert(app_config.ringback_cnt>0);
		if (--app_config.ringback_cnt == 0 && 
			app_config.ringback_slot!=PJSUA_INVALID_ID) 
		{
			pjsua_conf_disconnect(app_config.ringback_slot, 0);
			pjmedia_tonegen_rewind(app_config.ringback_port);
		}
    }

    if (app_config.call_data[call_id].ring_on) {
		app_config.call_data[call_id].ring_on = PJ_FALSE;

		pj_assert(app_config.ring_cnt>0);
		if (--app_config.ring_cnt == 0 && 
			app_config.ring_slot!=PJSUA_INVALID_ID) 
		{
			pjsua_conf_disconnect(app_config.ring_slot, 0);
			pjmedia_tonegen_rewind(app_config.ring_port);
		}
    }
}

void CPJSIPDll::ring_start(pjsua_call_id call_id)
{
    if (app_config.no_tones)
		return;

    if (app_config.call_data[call_id].ring_on)
		return;

    app_config.call_data[call_id].ring_on = PJ_TRUE;

    if (++app_config.ring_cnt==1 && 
		app_config.ring_slot!=PJSUA_INVALID_ID) 
    {
		pjsua_conf_connect(app_config.ring_slot, 0);
    }
}

/*
 * Find next call when current call is disconnected 
 */
pj_bool_t CPJSIPDll::find_next_call(void)
{
    int i, max;

    max = pjsua_call_get_max_count();
    for (i=current_call+1; i<max; ++i) {
		if (pjsua_call_is_active(i)) {
			current_call = i;
			return PJ_TRUE;
		}
    }

    for (i=0; i<current_call; ++i) {
		if (pjsua_call_is_active(i)) {
			current_call = i;
			return PJ_TRUE;
		}
    }

    current_call = PJSUA_INVALID_ID;
    return PJ_FALSE;
}


/*
 * Find previous call
 */
pj_bool_t CPJSIPDll::find_prev_call(void)
{
    int i, max;

    max = pjsua_call_get_max_count();
    for (i=current_call-1; i>=0; --i) {
		if (pjsua_call_is_active(i)) {
			current_call = i;
			return PJ_TRUE;
		}
    }

    for (i=max-1; i>current_call; --i) {
		if (pjsua_call_is_active(i)) {
			current_call = i;
			return PJ_TRUE;
		}
    }

    current_call = PJSUA_INVALID_ID;
    return PJ_FALSE;
}

/* Callback from timer when the maximum call duration has been
 * exceeded.
 */
void CPJSIPDll::call_timeout_callback(pj_timer_heap_t *timer_heap,
				  struct pj_timer_entry *entry)
{
    pjsua_call_id call_id = entry->id;
    pjsua_msg_data msg_data;
    pjsip_generic_string_hdr warn;
    pj_str_t hname = pj_str((char *)"Warning");
    pj_str_t hvalue = pj_str((char *)"399 pjsua \"Call duration exceeded\"");

    PJ_UNUSED_ARG(timer_heap);

	if(call_id == PJSUA_INVALID_ID) {
	PJ_LOG(1,(THIS_FILE, "Invalid call ID in timer callback"));
	return;
    }
    
    /* Add warning header */
    pjsua_msg_data_init(&msg_data);
    pjsip_generic_string_hdr_init2(&warn, &hname, &hvalue);
    pj_list_push_back(&msg_data.hdr_list, &warn);

    /* Call duration has been exceeded; disconnect the call */
    PJ_LOG(3,(THIS_FILE, "Duration (%d seconds) has been exceeded "
			 "for call %d, disconnecting the call",
			 app_config.duration, call_id));
    entry->id = PJSUA_INVALID_ID;
    pjsua_call_hangup(call_id, 200, NULL, &msg_data);
}


/*
 * Handler when invite state has changed.
 */
void CPJSIPDll::on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
    pjsua_call_info call_info;

    PJ_UNUSED_ARG(e);

    pjsua_call_get_info(call_id, &call_info);

    if (call_info.state == PJSIP_INV_STATE_DISCONNECTED) {

	char event_log[200];
	pj_memset(event_log, 0, sizeof(event_log));

	/* Stop all ringback for this call */
	ring_stop(call_id);

	/* Cancel duration timer, if any */
	if (app_config.call_data[call_id].timer.id != PJSUA_INVALID_ID) {
	    struct call_data *cd = &app_config.call_data[call_id];
	    pjsip_endpoint *endpt = pjsua_get_pjsip_endpt();

	    cd->timer.id = PJSUA_INVALID_ID;
	    pjsip_endpt_cancel_timer(endpt, &cd->timer);
	}

	/* Rewind play file when hangup automatically, 
	 * since file is not looped
	 */
	if (app_config.auto_play_hangup)
	    pjsua_player_set_pos(app_config.wav_id, 0);


	PJ_LOG(3,(THIS_FILE, "Call %d is DISCONNECTED [reason=%d (%s)]", 
		  call_id,
		  call_info.last_status,
		  call_info.last_status_text.ptr));
	sprintf(event_log, "Call %d is DISCONNECTED [reason=%d (%s)]", 
		  call_id,
		  call_info.last_status,
		  call_info.last_status_text.ptr);
	pj_dll_event_write(4, (const char *)event_log, strlen(event_log));

	pjsip_endpt_cancel_timer(pjsua_get_pjsip_endpt(), &qosTimer);
	update_call_setting();
	if (call_id == current_call) {
	    find_next_call();
	}

	/* Dump media state upon disconnected */
	if (1) {
	    PJ_LOG(5,(THIS_FILE, 
		      "Call %d disconnected, dumping media stats..", 
		      call_id));
	    log_call_dump(call_id);
	}

    } else {

	if (app_config.duration!=NO_LIMIT && 
	    call_info.state == PJSIP_INV_STATE_CONFIRMED) 
	{
	    /* Schedule timer to hangup call after the specified duration */
	    struct call_data *cd = &app_config.call_data[call_id];
	    pjsip_endpoint *endpt = pjsua_get_pjsip_endpt();
	    pj_time_val delay;

	    cd->timer.id = call_id;
	    delay.sec = app_config.duration;
	    delay.msec = 0;
	    pjsip_endpt_schedule_timer(endpt, &cd->timer, &delay);
	}

	if (call_info.state == PJSIP_INV_STATE_EARLY) {
	    int code;
	    pj_str_t reason;
	    pjsip_msg *msg;

	    /* This can only occur because of TX or RX message */
	    pj_assert(e->type == PJSIP_EVENT_TSX_STATE);

	    if (e->body.tsx_state.type == PJSIP_EVENT_RX_MSG) {
		msg = e->body.tsx_state.src.rdata->msg_info.msg;
	    } else {
		msg = e->body.tsx_state.src.tdata->msg;
	    }

	    code = msg->line.status.code;
	    reason = msg->line.status.reason;

	    /* Start ringback for 180 for UAC unless there's SDP in 180 */
	    if (call_info.role==PJSIP_ROLE_UAC && code==180 && 
		msg->body == NULL && 
		call_info.media_status==PJSUA_CALL_MEDIA_NONE) 
	    {
		ringback_start(call_id);
	    }

	    PJ_LOG(3,(THIS_FILE, "Call %d state changed to %s (%d %.*s)", 
		      call_id, call_info.state_text.ptr,
		      code, (int)reason.slen, reason.ptr));
	} else {
	    PJ_LOG(3,(THIS_FILE, "Call %d state changed to %s", 
		      call_id,
		      call_info.state_text.ptr));
	}

	if (current_call==PJSUA_INVALID_ID)
	    current_call = call_id;

    }
	if (call_info.state == PJSIP_INV_STATE_CONFIRMED) {
		cbParams->res = RES_CONFIRMED;
		Dialer_Confirmed();
	}
}



/*
 * Handler when a transaction within a call has changed state.
 */
void CPJSIPDll::on_call_tsx_state(pjsua_call_id call_id,
			      pjsip_transaction *tsx,
			      pjsip_event *e)
{
    const pjsip_method info_method = 
    {
		PJSIP_OTHER_METHOD,
		{ (char *)"INFO", 4 }
    };

    if (pjsip_method_cmp(&tsx->method, &info_method)==0) {
		/*
		* Handle INFO method.
		*/
		const pj_str_t STR_APPLICATION = { (char *)"application", 11};
		const pj_str_t STR_DTMF_RELAY  = { (char *)"dtmf-relay", 10 };
		pjsip_msg_body *body = NULL;
		pj_bool_t dtmf_info = PJ_FALSE;
	
		if (tsx->role == PJSIP_ROLE_UAC) {
			if (e->body.tsx_state.type == PJSIP_EVENT_TX_MSG)
				body = e->body.tsx_state.src.tdata->msg->body;
			else
			body = e->body.tsx_state.tsx->last_tx->msg->body;
		} else {
			if (e->body.tsx_state.type == PJSIP_EVENT_RX_MSG)
				body = e->body.tsx_state.src.rdata->msg_info.msg->body;
		}
	
		/* Check DTMF content in the INFO message */
		if (body && body->len &&
			pj_stricmp(&body->content_type.type, &STR_APPLICATION)==0 &&
			pj_stricmp(&body->content_type.subtype, &STR_DTMF_RELAY)==0)
		{
			dtmf_info = PJ_TRUE;
		}

		if (dtmf_info && tsx->role == PJSIP_ROLE_UAC && 
			(tsx->state == PJSIP_TSX_STATE_COMPLETED ||
			(tsx->state == PJSIP_TSX_STATE_TERMINATED &&
	        e->body.tsx_state.prev_state != PJSIP_TSX_STATE_COMPLETED))) 
		{
			/* Status of outgoing INFO request */
			if (tsx->status_code >= 200 && tsx->status_code < 300) {
				PJ_LOG(4,(THIS_FILE, 
					"Call %d: DTMF sent successfully with INFO",
					call_id));
			} else if (tsx->status_code >= 300) {
				PJ_LOG(4,(THIS_FILE, 
					"Call %d: Failed to send DTMF with INFO: %d/%.*s",
					call_id,
					tsx->status_code,
				(int)tsx->status_text.slen,
				tsx->status_text.ptr));
			}
		} else if (dtmf_info && tsx->role == PJSIP_ROLE_UAS &&
		   tsx->state == PJSIP_TSX_STATE_TRYING)
			{
				/* Answer incoming INFO with 200/OK */
				pjsip_rx_data *rdata;
				pjsip_tx_data *tdata;
				pj_status_t status;

				rdata = e->body.tsx_state.src.rdata;

				if (rdata->msg_info.msg->body) {
					status = pjsip_endpt_create_response(tsx->endpt, rdata,
						     200, NULL, &tdata);
					if (status == PJ_SUCCESS)
						status = pjsip_tsx_send_msg(tsx, tdata);

					PJ_LOG(3,(THIS_FILE, "Call %d: incoming INFO:\n%.*s", 
						call_id,
						(int)rdata->msg_info.msg->body->len,
						rdata->msg_info.msg->body->data));
			} else {
				status = pjsip_endpt_create_response(tsx->endpt, rdata,
						     400, NULL, &tdata);
				if (status == PJ_SUCCESS)
					status = pjsip_tsx_send_msg(tsx, tdata);
			}
		}
    }
}

/* General processing for media state. "mi" is the media index */
void CPJSIPDll::on_call_generic_media_state(pjsua_call_info *ci, unsigned mi,
                                        pj_bool_t *has_error)
{
    const char *status_name[] = {
        "None",
        "Active",
        "Local hold",
        "Remote hold",
        "Error"
    };

	const char *type_names[] = {
	"none",
	"audio",
	"video",
	"application",
	"unknown"
    };

    PJ_UNUSED_ARG(has_error);

    pj_assert(ci->media[mi].status <= PJ_ARRAY_SIZE(status_name));
    pj_assert(PJSUA_CALL_MEDIA_ERROR == 4);

    PJ_LOG(4,(THIS_FILE, "Call %d media %d [type=%s], status is %s",
	      ci->id, mi, type_names[ci->media[mi].type],
	      status_name[ci->media[mi].status]));
}

/* Process audio media state. "mi" is the media index. */
void CPJSIPDll::on_call_audio_state(pjsua_call_info *ci, unsigned mi,
                                pj_bool_t *has_error)
{
    PJ_UNUSED_ARG(has_error);

    /* Stop ringback */
    ring_stop(ci->id);

    /* Connect ports appropriately when media status is ACTIVE or REMOTE HOLD,
     * otherwise we should NOT connect the ports.
     */
    if (ci->media[mi].status == PJSUA_CALL_MEDIA_ACTIVE ||
	ci->media[mi].status == PJSUA_CALL_MEDIA_REMOTE_HOLD)
    {
	pj_bool_t connect_sound = PJ_TRUE;
	pj_bool_t disconnect_mic = PJ_FALSE;
	pjsua_conf_port_id call_conf_slot;

	call_conf_slot = ci->media[mi].stream.aud.conf_slot;

	/* Loopback sound, if desired */
	if (app_config.auto_loop) {
	    pjsua_conf_connect(call_conf_slot, call_conf_slot);
	    connect_sound = PJ_FALSE;
	}

	/* Automatically record conversation, if desired */
	if (app_config.auto_rec && app_config.rec_port != PJSUA_INVALID_ID) {
	    pjsua_conf_connect(call_conf_slot, app_config.rec_port);
	}

	/* Stream a file, if desired */
	if ((app_config.auto_play || app_config.auto_play_hangup) && 
	    app_config.wav_port != PJSUA_INVALID_ID)
	{
	    pjsua_conf_connect(app_config.wav_port, call_conf_slot);
	    connect_sound = PJ_FALSE;
	}

	/* Stream AVI, if desired */
	if (app_config.avi_auto_play &&
	    app_config.avi_def_idx != PJSUA_INVALID_ID &&
	    app_config.avi[app_config.avi_def_idx].slot != PJSUA_INVALID_ID)
	{
	    pjsua_conf_connect(app_config.avi[app_config.avi_def_idx].slot,
			       call_conf_slot);
	    disconnect_mic = PJ_TRUE;
	}

	/* Put call in conference with other calls, if desired */
	if (app_config.auto_conf) {
	    pjsua_call_id call_ids[PJSUA_MAX_CALLS];
	    unsigned call_cnt=PJ_ARRAY_SIZE(call_ids);
	    unsigned i;

	    /* Get all calls, and establish media connection between
	     * this call and other calls.
	     */
	    pjsua_enum_calls(call_ids, &call_cnt);

	    for (i=0; i<call_cnt; ++i) {
		if (call_ids[i] == ci->id)
		    continue;
		
		if (!pjsua_call_has_media(call_ids[i]))
		    continue;

		pjsua_conf_connect(call_conf_slot,
				   pjsua_call_get_conf_port(call_ids[i]));
		pjsua_conf_connect(pjsua_call_get_conf_port(call_ids[i]),
		                   call_conf_slot);

		/* Automatically record conversation, if desired */
		if (app_config.auto_rec && app_config.rec_port != PJSUA_INVALID_ID) {
		    pjsua_conf_connect(pjsua_call_get_conf_port(call_ids[i]), 
				       app_config.rec_port);
		}

	    }

	    /* Also connect call to local sound device */
	    connect_sound = PJ_TRUE;
	}

	/* Otherwise connect to sound device */
	if (connect_sound) {
	    pjsua_conf_connect(call_conf_slot, 0);
	    if (!disconnect_mic)
		pjsua_conf_connect(0, call_conf_slot);

	    /* Automatically record conversation, if desired */
	    if (app_config.auto_rec && app_config.rec_port != PJSUA_INVALID_ID) {
		pjsua_conf_connect(call_conf_slot, app_config.rec_port);
		pjsua_conf_connect(0, app_config.rec_port);
	    }
	}
    }
}

/* arrange windows. arg:
 *   -1:    arrange all windows
 *   != -1: arrange only this window id
 */
void CPJSIPDll::arrange_window(pjsua_vid_win_id wid)
{
#if PJSUA_HAS_VIDEO
    pjmedia_coord pos;
    int i, last;

    pos.x = 0;
    pos.y = 10;
    last = (wid == PJSUA_INVALID_ID) ? PJSUA_MAX_VID_WINS : wid;

    for (i=0; i<last; ++i) {
	pjsua_vid_win_info wi;
	pj_status_t status;

	status = pjsua_vid_win_get_info(i, &wi);
	if (status != PJ_SUCCESS)
	    continue;

	if (wid == PJSUA_INVALID_ID)
	    pjsua_vid_win_set_pos(i, &pos);

	if (wi.show)
	    pos.y += wi.size.h;
    }

    if (wid != PJSUA_INVALID_ID)
	pjsua_vid_win_set_pos(wid, &pos);
#else
    PJ_UNUSED_ARG(wid);
#endif
}

/* Process video media state. "mi" is the media index. */
void CPJSIPDll::on_call_video_state(pjsua_call_info *ci, unsigned mi,
                                pj_bool_t *has_error)
{
    if (ci->media_status != PJSUA_CALL_MEDIA_ACTIVE)
	return;
	
    arrange_window(ci->media[mi].stream.vid.win_in);

    PJ_UNUSED_ARG(has_error);
}

/*
 * Callback on media state changed event.
 * The action may connect the call to sound device, to file, or
 * to loop the call.
 */
void CPJSIPDll::on_call_media_state(pjsua_call_id call_id)
{
    pjsua_call_info call_info;
    unsigned mi;
    pj_bool_t has_error = PJ_FALSE;

    pjsua_call_get_info(call_id, &call_info);

    for (mi=0; mi<call_info.media_cnt; ++mi) {
	on_call_generic_media_state(&call_info, mi, &has_error);

	switch (call_info.media[mi].type) {
	case PJMEDIA_TYPE_AUDIO:
		PJ_LOG(4, (THIS_FILE, "audio status = %d", call_info.media[mi].status));
	    on_call_audio_state(&call_info, mi, &has_error);
	    break;
	case PJMEDIA_TYPE_VIDEO:
	    on_call_video_state(&call_info, mi, &has_error);
	    break;
	default:
	    /* Make gcc happy about enum not handled by switch/case */
	    break;
	}
    }

    if (has_error) {
	pj_str_t reason = pj_str((char *)"Media failed");
	pjsua_call_hangup(call_id, 500, &reason, NULL);
    }

#if PJSUA_HAS_VIDEO
    /* Check if remote has just tried to enable video */
    //if (call_info.rem_offerer && call_info.rem_vid_cnt)
	if (call_info.state == PJSIP_INV_STATE_CONFIRMED)
    {
		int vid_idx;
		pjsua_stream_info si;
		char codec_id[32];
		pj_str_t cid;
		pjmedia_vid_codec_param cp;

		/* Check if there is active video */
		vid_idx = pjsua_call_get_vid_stream_idx(call_id);
		/*if (vid_idx == -1 || call_info.media[vid_idx].dir == PJMEDIA_DIR_NONE) {
			PJ_LOG(3,(THIS_FILE,
		      "Just rejected incoming video offer on call %d, "
		      "use \"vid call enable %d\" or \"vid call add\" to enable video!",
		      call_id, vid_idx));
		}*/
		if(pjsua_call_vid_stream_is_running(call_id, vid_idx, PJMEDIA_DIR_ENCODING_DECODING)) 
		{
		pjsua_call_get_stream_info(call_id, vid_idx, &si);
		if(si.info.vid.rx_pt == PJMEDIA_RTP_PT_H264) {
			sprintf_s(codec_id, "H264/%d", PJMEDIA_RTP_PT_H264);
			cid = pj_str(codec_id);
		}
		else if(si.info.vid.rx_pt == PJMEDIA_RTP_PT_H263P) {
			sprintf_s(codec_id, "H263P/%d", PJMEDIA_RTP_PT_H263P);
			cid = pj_str(codec_id);
		}
	
		status = pjsua_vid_codec_get_param(&cid, &cp);
		if (status == PJ_SUCCESS) {
			if(vbrDriver) {
				vbrDriver->set_original_avg_bps(cp.enc_fmt.det.vid.avg_bps);
				vbrDriver->set_original_max_bps(cp.enc_fmt.det.vid.max_bps);
			}
			if(qosAnalyzer) {
				qosAnalyzer->set_original_avg_bps(cp.enc_fmt.det.vid.avg_bps);
				qosAnalyzer->set_original_max_bps(cp.enc_fmt.det.vid.max_bps);
			}
			PJ_LOG(4, (THIS_FILE, "avg_bps = %d, max_bps = %d", cp.enc_fmt.det.vid.avg_bps, cp.enc_fmt.det.vid.max_bps));
		}
		}
    }
#endif
}

/*
 * DTMF callback.
 */
void CPJSIPDll::call_on_dtmf_callback(pjsua_call_id call_id, int dtmf)
{
	char event_log[100];
	pj_memset(event_log, 0, sizeof(event_log));
    PJ_LOG(4,(THIS_FILE, "Inside call_on_dtmf_callback: Incoming DTMF on call %d: %c", call_id, dtmf));
	sprintf(event_log,  "Incoming DTMF on call %d: %c", call_id, dtmf);
	pj_dll_event_write(4, (const char *)event_log, strlen(event_log));
}


/*
 * Handler registration status has changed.
 */
void CPJSIPDll::on_reg_state(pjsua_acc_id acc_id)
{
    PJ_UNUSED_ARG(acc_id);

    // Log already written.
}


/*
 * NAT type detection callback.
 */
void CPJSIPDll::on_nat_detect(const pj_stun_nat_detect_result *res)
{
    if (res->status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "NAT detection failed", res->status);
    } else {
	PJ_LOG(3, (THIS_FILE, "NAT detected as %s", res->nat_type_name));
    }
}


/*
 * MWI indication
 */
void CPJSIPDll::on_mwi_info(pjsua_acc_id acc_id, pjsua_mwi_info *mwi_info)
{
    pj_str_t body;
    
    PJ_LOG(3,(THIS_FILE, "Received MWI for acc %d:", acc_id));

    if (mwi_info->rdata->msg_info.ctype) {
	const pjsip_ctype_hdr *ctype = mwi_info->rdata->msg_info.ctype;

	PJ_LOG(3,(THIS_FILE, " Content-Type: %.*s/%.*s",
	          (int)ctype->media.type.slen,
		  ctype->media.type.ptr,
		  (int)ctype->media.subtype.slen,
		  ctype->media.subtype.ptr));
    }

    if (!mwi_info->rdata->msg_info.msg->body) {
	PJ_LOG(3,(THIS_FILE, "  no message body"));
	return;
    }

    body.ptr = (char *)(mwi_info->rdata->msg_info.msg->body->data);
    body.slen = mwi_info->rdata->msg_info.msg->body->len;

    PJ_LOG(3,(THIS_FILE, " Body:\n%.*s", (int)body.slen, body.ptr));
}


/*
 * Transport status notification
 */
void CPJSIPDll::on_transport_state(pjsip_transport *tp, 
			       pjsip_transport_state state,
			       const pjsip_transport_state_info *info)
{
    char host_port[128];

    pj_ansi_snprintf(host_port, sizeof(host_port), "[%.*s:%d]",
		     (int)tp->remote_name.host.slen,
		     tp->remote_name.host.ptr,
		     tp->remote_name.port);

    switch (state) {
    case PJSIP_TP_STATE_CONNECTED:
	{
	    PJ_LOG(3,(THIS_FILE, "SIP %s transport is connected to %s",
		     tp->type_name, host_port));
	}
	break;

    case PJSIP_TP_STATE_DISCONNECTED:
	{
	    char buf[100];

	    snprintf(buf, sizeof(buf), "SIP %s transport is disconnected from %s",
		     tp->type_name, host_port);
	    pjsua_perror(THIS_FILE, buf, info->status);
	}
	break;

    default:
	break;
    }

#if defined(PJSIP_HAS_TLS_TRANSPORT) && PJSIP_HAS_TLS_TRANSPORT!=0

    if (!pj_ansi_stricmp(tp->type_name, "tls") && info->ext_info &&
	(state == PJSIP_TP_STATE_CONNECTED || 
	 ((pjsip_tls_state_info*)info->ext_info)->
			         ssl_sock_info->verify_status != PJ_SUCCESS))
    {
	pjsip_tls_state_info *tls_info = (pjsip_tls_state_info*)info->ext_info;
	pj_ssl_sock_info *ssl_sock_info = tls_info->ssl_sock_info;
	char buf[2048];
	const char *verif_msgs[32];
	unsigned verif_msg_cnt;

	/* Dump server TLS cipher */
	PJ_LOG(4,(THIS_FILE, "TLS cipher used: 0x%06X/%s",
		  ssl_sock_info->cipher,
		  pj_ssl_cipher_name(ssl_sock_info->cipher) ));

	/* Dump server TLS certificate */
	pj_ssl_cert_info_dump(ssl_sock_info->remote_cert_info, "  ",
			      buf, sizeof(buf));
	PJ_LOG(4,(THIS_FILE, "TLS cert info of %s:\n%s", host_port, buf));

	/* Dump server TLS certificate verification result */
	verif_msg_cnt = PJ_ARRAY_SIZE(verif_msgs);
	pj_ssl_cert_get_verify_status_strings(ssl_sock_info->verify_status,
					      verif_msgs, &verif_msg_cnt);
	PJ_LOG(3,(THIS_FILE, "TLS cert verification result of %s : %s",
			     host_port,
			     (verif_msg_cnt == 1? verif_msgs[0]:"")));
	if (verif_msg_cnt > 1) {
	    unsigned i;
	    for (i = 0; i < verif_msg_cnt; ++i)
		PJ_LOG(3,(THIS_FILE, "- %s", verif_msgs[i]));
	}

	if (ssl_sock_info->verify_status &&
	    !app_config.udp_cfg.tls_setting.verify_server) 
	{
	    PJ_LOG(3,(THIS_FILE, "PJSUA is configured to ignore TLS cert "
				 "verification errors"));
	}
    }

#endif

}

/*
 * Notification on ICE error.
 */
void CPJSIPDll::on_ice_transport_error(int index, pj_ice_strans_op op,
				   pj_status_t status, void *param)
{
    PJ_UNUSED_ARG(op);
    PJ_UNUSED_ARG(param);
    PJ_PERROR(1,(THIS_FILE, status,
	         "ICE keep alive failure for transport %d", index));
}

/*
 * Notification on sound device operation.
 */
pj_status_t CPJSIPDll::on_snd_dev_operation(int operation)
{
    PJ_LOG(3,(THIS_FILE, "Turning sound device %s", (operation? "ON":"OFF")));
    return PJ_SUCCESS;
}

/* Callback on media events */
void CPJSIPDll::on_call_media_event(pjsua_call_id call_id,
                                unsigned med_idx,
                                pjmedia_event *event)
{
    char event_name[5];

    PJ_LOG(4,(THIS_FILE, "Event %s",
	      pjmedia_fourcc_name(event->type, event_name)));

#if PJSUA_HAS_VIDEO
    if (event->type == PJMEDIA_EVENT_FMT_CHANGED) {
		/* Adjust renderer window size to original video size */
		pjsua_call_info ci;
		pjsua_vid_win_id wid;
		pjmedia_rect_size size;

		pjsua_call_get_info(call_id, &ci);

		if ((ci.media[med_idx].type == PJMEDIA_TYPE_VIDEO) &&
			(ci.media[med_idx].dir & PJMEDIA_DIR_DECODING))
		{
			//todo: ivan look here :)
			pjmedia_rect_size win_size;
			wid = ci.media[med_idx].stream.vid.win_in;
			size = event->data.fmt_changed.new_fmt.det.vid.size;
			// say the window width is 640.
			win_size.w = 640;
			// window size must reflect incoming video aspect ratio.
			win_size.h = (win_size.w * size.h)/size.w;
			//pjsua_vid_win_set_size(wid, &win_size);
			pjsua_vid_win_set_size(wid, &size);
			PJ_LOG(4,(THIS_FILE, "Window size set to, w:%d h:%d", size.w, size.h));
		}

		/* Re-arrange video windows */
		arrange_window(PJSUA_INVALID_ID);
    }
#else
    PJ_UNUSED_ARG(call_id);
    PJ_UNUSED_ARG(med_idx);
    PJ_UNUSED_ARG(event);
#endif
}


/* Playfile done notification, set timer to hangup calls */
pj_status_t CPJSIPDll::on_playfile_done(pjmedia_port *port, void *usr_data)
{
    pj_time_val delay;

    PJ_UNUSED_ARG(port);
    PJ_UNUSED_ARG(usr_data);

    /* Just rewind WAV when it is played outside of call */
    if (pjsua_call_get_count() == 0) {
		pjsua_player_set_pos(app_config.wav_id, 0);
		return PJ_SUCCESS;
    }

    /* Timer is already active */
    if (app_config.auto_hangup_timer.id == 1)
		return PJ_SUCCESS;

    app_config.auto_hangup_timer.id = 1;
    delay.sec = 0;
    delay.msec = 200; /* Give 200 ms before hangup */
    pjsip_endpt_schedule_timer(pjsua_get_pjsip_endpt(), 
			       &app_config.auto_hangup_timer, 
			       &delay);

    return PJ_SUCCESS;
}

/* Auto hangup timer callback */
void CPJSIPDll::hangup_timeout_callback(pj_timer_heap_t *timer_heap,
				    struct pj_timer_entry *entry)
{
    PJ_UNUSED_ARG(timer_heap);
    PJ_UNUSED_ARG(entry);

    app_config.auto_hangup_timer.id = 0;
    pjsua_call_hangup_all();
}


void CPJSIPDll::app_config_init_video(pjsua_acc_config *acc_cfg)
{
    acc_cfg->vid_in_auto_show = app_config.vid.in_auto_show;
    acc_cfg->vid_out_auto_transmit = app_config.vid.out_auto_transmit;
    /* Note that normally GUI application will prefer a borderless
     * window.
     */
    acc_cfg->vid_wnd_flags = PJMEDIA_VID_DEV_WND_BORDER |
                             PJMEDIA_VID_DEV_WND_RESIZABLE;
    acc_cfg->vid_cap_dev = app_config.vid.vcapture_dev;
    acc_cfg->vid_rend_dev = app_config.vid.vrender_dev;

    if (app_config.avi_auto_play &&
	app_config.avi_def_idx != PJSUA_INVALID_ID &&
	app_config.avi[app_config.avi_def_idx].dev_id != PJMEDIA_VID_INVALID_DEV)
    {
	acc_cfg->vid_cap_dev = app_config.avi[app_config.avi_def_idx].dev_id;
    }
}


void CPJSIPDll::vid_handle_menu(int action, char *param)
{
	pj_bool_t enabled;
	
	if(action & VID_ENABLE) {
		enabled = PJ_TRUE;
		app_config.vid.vid_cnt = (enabled ? 1 : 0);
	}
	else if(action & VID_DISABLE ) {
		enabled = PJ_FALSE;
		app_config.vid.vid_cnt = (enabled ? 1 : 0);
	}
	pjsua_acc_config acc_cfg;
	pj_bool_t changed = PJ_FALSE;
	int on = 1;
	pjsua_acc_get_config(current_acc, &acc_cfg);
	if(action & VID_ACC_AUTORX_ON) {
		acc_cfg.vid_in_auto_show = on;
	    changed = PJ_TRUE;
	}
	else if(action & VID_ACC_AUTORX_OFF) {
		acc_cfg.vid_in_auto_show = 0;
	    changed = PJ_TRUE;
	}
	if(action & VID_ACC_AUTOTX_ON) {
		acc_cfg.vid_out_auto_transmit = on;
	    changed = PJ_TRUE;
	}
	else if(action & VID_ACC_AUTOTX_OFF) {
		acc_cfg.vid_out_auto_transmit = 0;
	    changed = PJ_TRUE;
	}
	if(action & VID_SET_CAP_DEV) {
		int dev = atoi(param);
	    acc_cfg.vid_cap_dev = dev;
	    changed = PJ_TRUE;
	}
	if(action & VID_SET_REND_DEV) {
		int dev = atoi(param);
	    acc_cfg.vid_cap_dev = dev;
	    changed = PJ_TRUE;
	}
	if (changed) {
	    pj_status_t status = pjsua_acc_modify(current_acc, &acc_cfg);
	    if (status != PJ_SUCCESS)
		PJ_PERROR(1,(THIS_FILE, status, "Error modifying account %d",
			     current_acc));
	}
}

pj_status_t CPJSIPDll::app_destroy(void)
{
    
    unsigned i;

#ifdef STEREO_DEMO
    if (app_config.snd) {
	pjmedia_snd_port_destroy(app_config.snd);
	app_config.snd = NULL;
    }
    if (app_config.sc_ch1) {
	pjsua_conf_remove_port(app_config.sc_ch1_slot);
	app_config.sc_ch1_slot = PJSUA_INVALID_ID;
	pjmedia_port_destroy(app_config.sc_ch1);
	app_config.sc_ch1 = NULL;
    }
    if (app_config.sc) {
	pjmedia_port_destroy(app_config.sc);
	app_config.sc = NULL;
    }
#endif

    /* Close avi devs and ports */
    for (i=0; i<app_config.avi_cnt; ++i) {
	if (app_config.avi[i].slot != PJSUA_INVALID_ID)
	    pjsua_conf_remove_port(app_config.avi[i].slot);
#if PJMEDIA_HAS_VIDEO && PJMEDIA_VIDEO_DEV_HAS_AVI
	if (app_config.avi[i].dev_id != PJMEDIA_VID_INVALID_DEV)
	    pjmedia_avi_dev_free(app_config.avi[i].dev_id);
#endif
    }

    /* Close ringback port */
    if (app_config.ringback_port && 
	app_config.ringback_slot != PJSUA_INVALID_ID) 
    {
		pjsua_conf_remove_port(app_config.ringback_slot);
		app_config.ringback_slot = PJSUA_INVALID_ID;
		pjmedia_port_destroy(app_config.ringback_port);
		app_config.ringback_port = NULL;
    }

    /* Close ring port */
    if (app_config.ring_port && app_config.ring_slot != PJSUA_INVALID_ID) {
		pjsua_conf_remove_port(app_config.ring_slot);
		app_config.ring_slot = PJSUA_INVALID_ID;
		pjmedia_port_destroy(app_config.ring_port);
		app_config.ring_port = NULL;
    }

    /* Close tone generators */
    for (i=0; i<app_config.tone_count; ++i) {
		pjsua_conf_remove_port(app_config.tone_slots[i]);
    }

    if (app_config.pool) {
		pj_pool_release(app_config.pool);
		app_config.pool = NULL;
    }

    status = pjsua_destroy();

    pj_bzero(&app_config, sizeof(app_config));
	//pj_thread_sleep(100);
    return status;
}

/* Display error and exit application */
void CPJSIPDll::error_exit(const char *title, pj_status_t status)
{
    pjsua_perror(THIS_FILE, title, status);
   // pjsua_destroy();
   // exit(1);
}

/*
 * A simple registrar, invoked by default_mod_on_rx_request()
 */
void CPJSIPDll::simple_registrar(pjsip_rx_data *rdata)
{
	pjsip_tx_data *tdata;
    const pjsip_expires_hdr *exp;
    const pjsip_hdr *h;
    unsigned cnt = 0;
    pjsip_generic_string_hdr *srv;

    status = pjsip_endpt_create_response(pjsua_get_pjsip_endpt(),
				 rdata, 200, NULL, &tdata);
    if (status != PJ_SUCCESS)
    return;

    exp = (const pjsip_expires_hdr *)pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_EXPIRES, NULL);

    h = rdata->msg_info.msg->hdr.next;
    while (h != &rdata->msg_info.msg->hdr) {
		if (h->type == PJSIP_H_CONTACT) {
			const pjsip_contact_hdr *c = (const pjsip_contact_hdr*)h;
			int e = c->expires;

			if (e < 0) {
				if (exp)
					e = exp->ivalue;
				else
					e = 3600;
			}

			if (e > 0) {
				pjsip_contact_hdr *nc = (pjsip_contact_hdr *)(pjsip_hdr_clone(tdata->pool, h));
				nc->expires = e;
				pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr*)nc);
				++cnt;
			}
		}
		h = h->next;
    }

    srv = pjsip_generic_string_hdr_create(tdata->pool, NULL, NULL);
    srv->name = pj_str((char *)"Server");
    srv->hvalue = pj_str((char *)"pjsua simple registrar");
    pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr*)srv);

    pjsip_endpt_send_response2(pjsua_get_pjsip_endpt(),
		       rdata, tdata, NULL, NULL);
}



/*****************************************************************************
 * A simple module to handle otherwise unhandled request. We will register
 * this with the lowest priority.
 */

/* Notification on incoming request */
pj_bool_t CPJSIPDll::default_mod_on_rx_request(pjsip_rx_data *rdata)
{
	pjsip_tx_data *tdata;
    pjsip_status_code status_code;

    /* Don't respond to ACK! */
    if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method,
			 &pjsip_ack_method) == 0)
	return PJ_TRUE;

    /* Simple registrar */
    if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method,
                         &pjsip_register_method) == 0)
    {
	simple_registrar(rdata);
	return PJ_TRUE;
    }

    /* Create basic response. */
    if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method, 
			 &pjsip_notify_method) == 0)
    {
	/* Unsolicited NOTIFY's, send with Bad Request */
	status_code = PJSIP_SC_BAD_REQUEST;
    } else {
	/* Probably unknown method */
	status_code = PJSIP_SC_METHOD_NOT_ALLOWED;
    }
    status = pjsip_endpt_create_response(pjsua_get_pjsip_endpt(), 
					 rdata, status_code, 
					 NULL, &tdata);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Unable to create response", status);
	return PJ_TRUE;
    }

    /* Add Allow if we're responding with 405 */
    if (status_code == PJSIP_SC_METHOD_NOT_ALLOWED) {
	const pjsip_hdr *cap_hdr;
	cap_hdr = pjsip_endpt_get_capability(pjsua_get_pjsip_endpt(), 
					     PJSIP_H_ALLOW, NULL);
	if (cap_hdr) {
	    pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr *)(pjsip_hdr_clone(tdata->pool, 
							   cap_hdr)));
	}
    }

    /* Add User-Agent header */
    {
	pj_str_t user_agent;
	char tmp[80];
	const pj_str_t USER_AGENT = { (char *)"User-Agent", 10};
	pjsip_hdr *h;

	pj_ansi_snprintf(tmp, sizeof(tmp), "PJSUA v%s/%s", 
			 pj_get_version(), PJ_OS_NAME);
	pj_strdup2_with_null(tdata->pool, &user_agent, tmp);

	h = (pjsip_hdr*) pjsip_generic_string_hdr_create(tdata->pool,
							 &USER_AGENT,
							 &user_agent);
	pjsip_msg_add_hdr(tdata->msg, h);
    }

    pjsip_endpt_send_response2(pjsua_get_pjsip_endpt(), rdata, tdata, 
			       NULL, NULL);

    return PJ_TRUE;
}

void CPJSIPDll::update_call_setting()
{
/* Update call setting */
	pjsua_call_setting_default(&call_opt);
	call_opt.aud_cnt = app_config.aud_cnt;
	call_opt.vid_cnt = app_config.vid.vid_cnt;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks


// initialize the functor objects using lambdas
std::function<pj_bool_t (pjsip_rx_data *rdata)> CPJSIPDll::s_func_on_rx_request = [](pjsip_rx_data *rdata) {return PJ_TRUE;} ;
std::function< void (pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry) > CPJSIPDll::s_func_call_timeout_callback = [](pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry) -> void {std::cout << "Test" << std::endl;};
std::function<void (pjsua_call_id call_id, pjsip_event *e)> CPJSIPDll::s_func_on_call_state = [](pjsua_call_id call_id, pjsip_event *e) {};
std::function<void (pjsua_call_id call_id, pjsip_transaction *tsx, pjsip_event *e)> CPJSIPDll::s_func_on_call_tsx_state = [](pjsua_call_id call_id, pjsip_transaction *tsx, pjsip_event *e) {};
std::function<void (pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error)> CPJSIPDll::s_func_on_call_generic_media_state = [](pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error) {};
std::function<void (pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error)> CPJSIPDll::s_func_on_call_audio_state = [](pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error) {};
std::function<void (pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error)> CPJSIPDll::s_func_on_call_video_state = [](pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error) {};
std::function<void (pjsua_call_id call_id)> CPJSIPDll::s_func_on_call_media_state = [](pjsua_call_id call_id) {};
std::function<void (pjsua_call_id call_id, int dtmf)> CPJSIPDll::s_func_call_on_dtmf_callback = [](pjsua_call_id call_id, int dtmf) {};
std::function<void (pjsua_acc_id acc_id)> CPJSIPDll::s_func_on_reg_state = [](pjsua_acc_id acc_id) {};
std::function<void (const pj_stun_nat_detect_result *res)> CPJSIPDll::s_func_on_nat_detect = [](const pj_stun_nat_detect_result *res) {};
std::function<void (pjsua_acc_id acc_id, pjsua_mwi_info *mwi_info)> CPJSIPDll::s_func_on_mwi_info = [](pjsua_acc_id acc_id, pjsua_mwi_info *mwi_info) {};
std::function<void (pjsip_transport *tp, pjsip_transport_state state, const pjsip_transport_state_info *info)> CPJSIPDll::s_func_on_transport_state = [](pjsip_transport *tp, pjsip_transport_state state, const pjsip_transport_state_info *info) {};
std::function<void (int index, pj_ice_strans_op op, pj_status_t status, void *param)> CPJSIPDll::s_func_on_ice_transport_error = [](int index, pj_ice_strans_op op, pj_status_t status, void *param) {};
std::function<pj_status_t (int operation)> CPJSIPDll::s_func_on_snd_dev_operation = [](int operation) { return PJ_SUCCESS; };
std::function<void (pjsua_call_id call_id, unsigned med_idx, pjmedia_event *event)> CPJSIPDll::s_func_on_call_media_event = [](pjsua_call_id call_id, unsigned med_idx, pjmedia_event *event) {};
std::function<pj_status_t (pjmedia_port *port, void *usr_data)> CPJSIPDll::s_func_on_playfile_done = [](pjmedia_port *port, void *usr_data) { return PJ_SUCCESS; };
std::function<void (pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)> CPJSIPDll::s_func_hangup_timeout_callback = [](pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry) {};

int CPJSIPDll::on_pjsip_dll_init()
{

	std::cout << "Inside on_pjsip_dll_init" << std::endl;
	std::cout << "cbParams: " << cbParams << std::endl;
	std::cout << "SIP Settings: " << cbParams->displayName << " " << cbParams->userName << " " << cbParams->sipProxyAddress << " " << cbParams->sipurl << " " << cbParams->password << std::endl;
	current_call = PJSUA_INVALID_ID;
	pjsua_transport_id transport_id = -1;
    pjsua_transport_config tcp_cfg;
    unsigned i;
    
	std::cout << "Inside on_pjsip_dll_init - 1a" << std::endl;
	pj_thread_desc desc = {0};
	pj_thread_register("PJSIPDll", desc, &thread);
	
	std::cout << "Inside on_pjsip_dll_init - 1b" << std::endl;
	app_restart = PJ_FALSE;

	/* Create pjsua */
	status = pjsua_create();
	if (status != PJ_SUCCESS)
		return status;
	std::cout << "Inside on_pjsip_dll_init - 1b1" << std::endl;
	/* Create pool for application */
    app_config.pool = pjsua_pool_create("PJSIPDll", 1000, 1000);

	std::cout << "Inside on_pjsip_dll_init - 1b2" << std::endl;
	/* Initialize default config */
	default_config();
	
	std::cout << "Inside on_pjsip_dll_init - 1c" << std::endl;
	if(cbParams->tcpOnly == 1) {
		app_config.no_udp = true;
	}
	app_config.udp_cfg.port = cbParams->sipPort;

	// Lower the log file size
	app_config.log_cfg.level = 4;

	// set num of worker threads to 0		
	app_config.cfg.thread_cnt = 0; // for POLLING
	app_config.media_cfg.thread_cnt = 0; // for POLLIN
	
	std::cout << "Inside on_pjsip_dll_init - 1d" << std::endl;
	// Set VAD flag
	//app_config.media_cfg.no_vad = PJ_FALSE;
	// Set EC tail length in ms
	app_config.media_cfg.ec_tail_len = 200;

#ifdef PJSIP_HAS_TLS_TRANSPORT
	app_config.use_tls = PJ_TRUE; 
	if (app_config.use_tls == PJ_TRUE)
	{
		//app_config->udp_cfg.tls_setting.ca_list_file = pj_str("");
		//app_config.udp_cfg.tls_setting.cert_file = pj_str("server.crt");
		//app_config.udp_cfg.tls_setting.privkey_file = pj_str("pkey.key");
	}
#endif

		// set stun address
		//TBD
		//app_config.cfg.stun_host = pj_str(ctx->stunAddress);
		// set nameserver address for DNS SRV support. Allow only 1 server.
		//if (strlen(ctx->nameServer) > 0) 
		//{
		//	app_config.cfg.nameserver_count = 1;
		//	app_config.cfg.nameserver[0] = pj_str(ctx->nameServer);
		//}
	

	std::cout << "Inside on_pjsip_dll_init - 1" << std::endl;
	/* Initialize application callbacks */
	set_on_call_state(std::bind(&CPJSIPDll::on_call_state, this, _1, _2));
	app_config.cfg.cb.on_call_state = &CPJSIPDll::forward_on_call_state;

	set_on_call_media_state(std::bind(&CPJSIPDll::on_call_media_state, this, _1));
    app_config.cfg.cb.on_call_media_state = &CPJSIPDll::forward_on_call_media_state;

	set_call_on_dtmf_callback(std::bind(&CPJSIPDll::call_on_dtmf_callback, this, _1, _2));
    app_config.cfg.cb.on_dtmf_digit = &CPJSIPDll::forward_call_on_dtmf_callback;

	set_on_call_tsx_state(std::bind(&CPJSIPDll::on_call_tsx_state, this, _1, _2, _3));
    app_config.cfg.cb.on_call_tsx_state = &CPJSIPDll::forward_on_call_tsx_state;

	set_on_reg_state(std::bind(&CPJSIPDll::on_reg_state, this, _1));
    app_config.cfg.cb.on_reg_state = &CPJSIPDll::forward_on_reg_state;

	set_on_nat_detect(std::bind(&CPJSIPDll::on_nat_detect, this, _1));
    app_config.cfg.cb.on_nat_detect =  &CPJSIPDll::forward_on_nat_detect;

	set_on_mwi_info(std::bind(&CPJSIPDll::on_mwi_info, this, _1, _2));
    app_config.cfg.cb.on_mwi_info = &CPJSIPDll::forward_on_mwi_info;

	set_on_transport_state(std::bind(&CPJSIPDll::on_transport_state, this, _1, _2, _3));
    app_config.cfg.cb.on_transport_state = &CPJSIPDll::forward_on_transport_state;

	set_on_ice_transport_error(std::bind(&CPJSIPDll::on_ice_transport_error, this, _1, _2, _3, _4));
    app_config.cfg.cb.on_ice_transport_error = &CPJSIPDll::forward_on_ice_transport_error;

	set_on_snd_dev_operation(std::bind(&CPJSIPDll::on_snd_dev_operation, this, _1));
    app_config.cfg.cb.on_snd_dev_operation =  &CPJSIPDll::forward_on_snd_dev_operation;

	set_on_call_media_event(std::bind(&CPJSIPDll::on_call_media_event, this, _1, _2, _3));
    app_config.cfg.cb.on_call_media_event = &CPJSIPDll::forward_on_call_media_event;

#ifdef TRANSPORT_ADAPTER_SAMPLE
	// TBD
    app_config.cfg.cb.on_create_media_transport = &on_create_media_transport;
#endif
    app_config.log_cfg.cb = log_cb;

    /* Set sound device latency */
    if (app_config.capture_lat > 0)
		app_config.media_cfg.snd_rec_latency = app_config.capture_lat;
    if (app_config.playback_lat)
		app_config.media_cfg.snd_play_latency = app_config.playback_lat;

	transport_id = -1;

    /* Initialize pjsua */
    status = pjsua_init(&app_config.cfg, &app_config.log_cfg,
			&app_config.media_cfg);
    if (status != PJ_SUCCESS)
		return status;

	std::cout << "Inside on_pjsip_dll_init - 2" << std::endl;
    /* Initialize our module to handle otherwise unhandled request */
    status = pjsip_endpt_register_module(pjsua_get_pjsip_endpt(),
					 &mod_default_handler);
    if (status != PJ_SUCCESS)
		return status;

	pj_log_set_log_func(&pj_dll_log_write);

    /* Initialize calls data */
    for (i=0; i<PJ_ARRAY_SIZE(app_config.call_data); ++i) {
		app_config.call_data[i].timer.id = PJSUA_INVALID_ID;
		set_call_timeout_callback(std::bind(&CPJSIPDll::call_timeout_callback, this, _1, _2));
		app_config.call_data[i].timer.cb = &CPJSIPDll::forward_call_timeout_callback;
    }

	std::cout << "Inside on_pjsip_dll_init - 3" << std::endl;
    /* Optionally registers WAV file */
    for (i=0; i<app_config.wav_count; ++i) {
		pjsua_player_id wav_id;
		unsigned play_options = 0;
		if (app_config.auto_play_hangup)
			play_options |= PJMEDIA_FILE_NO_LOOP;
		std::cout << "----3d1---: " << i << " " << app_config.wav_count << std::endl;
		status = pjsua_player_create(&app_config.wav_files[i], play_options, 
				     &wav_id);
		if (status != PJ_SUCCESS) {
			goto on_error;
		}

		if (app_config.wav_id == PJSUA_INVALID_ID) {
			
			app_config.wav_id = wav_id;
			app_config.wav_port = pjsua_player_get_conf_port(app_config.wav_id);
			
			if (app_config.auto_play_hangup) {
				pjmedia_port  *port;
				pjsua_player_get_port(app_config.wav_id, &port);
				set_on_playfile_done(std::bind(&CPJSIPDll::on_playfile_done, this, _1, _2));
				status = pjmedia_wav_player_set_eof_cb(port, NULL, 
						       &CPJSIPDll::forward_on_playfile_done);
				if (status != PJ_SUCCESS)
					goto on_error;

				set_hangup_timeout_callback(std::bind(&CPJSIPDll::hangup_timeout_callback, this, _1, _2));
				pj_timer_entry_init(&app_config.auto_hangup_timer, 0, NULL, 
						 &CPJSIPDll::forward_hangup_timeout_callback);
			}
		}
    }

	std::cout << "Inside on_pjsip_dll_init - 4" << std::endl;
    /* Optionally registers tone players */
    for (i=0; i<app_config.tone_count; ++i) {
		pjmedia_port *tport;
		char name[80];
		pj_str_t label;
		pj_status_t status;

		pj_ansi_snprintf(name, sizeof(name), "tone-%d,%d",
			 app_config.tones[i].freq1, 
			 app_config.tones[i].freq2);
		label = pj_str(name);
		status = pjmedia_tonegen_create2(app_config.pool, &label,
					 8000, 1, 160, 16, 
					 PJMEDIA_TONEGEN_LOOP,  &tport);
		if (status != PJ_SUCCESS) {
			pjsua_perror(THIS_FILE, "Unable to create tone generator", status);
			goto on_error;
		}

		status = pjsua_conf_add_port(app_config.pool, tport, 
				     &app_config.tone_slots[i]);
		pj_assert(status == PJ_SUCCESS);

		status = pjmedia_tonegen_play(tport, 1, &app_config.tones[i], 0);
		pj_assert(status == PJ_SUCCESS);
    }

    /* Optionally create recorder file, if any. */
    if (app_config.rec_file.slen) {
		status = pjsua_recorder_create(&app_config.rec_file, 0, NULL, 0, 0,
						   &app_config.rec_id);
		if (status != PJ_SUCCESS)
			goto on_error;

		app_config.rec_port = pjsua_recorder_get_conf_port(app_config.rec_id);
    }

    pj_memcpy(&tcp_cfg, &app_config.udp_cfg, sizeof(tcp_cfg));

    /* Create ringback tones */
    if (app_config.no_tones == PJ_FALSE) {
		unsigned i, samples_per_frame;
		pjmedia_tone_desc tone[RING_CNT+RINGBACK_CNT];
		pj_str_t name;

		samples_per_frame = app_config.media_cfg.audio_frame_ptime * 
					app_config.media_cfg.clock_rate *
					app_config.media_cfg.channel_count / 1000;

		/* Ringback tone (call is ringing) */
		name = pj_str((char *)"ringback");
		status = pjmedia_tonegen_create2(app_config.pool, &name, 
						 app_config.media_cfg.clock_rate,
						 app_config.media_cfg.channel_count, 
						 samples_per_frame,
						 16, PJMEDIA_TONEGEN_LOOP, 
						 &app_config.ringback_port);
		if (status != PJ_SUCCESS)
			goto on_error;

		pj_bzero(&tone, sizeof(tone));
		for (i=0; i<RINGBACK_CNT; ++i) {
			tone[i].freq1 = RINGBACK_FREQ1;
			tone[i].freq2 = RINGBACK_FREQ2;
			tone[i].on_msec = RINGBACK_ON;
			tone[i].off_msec = RINGBACK_OFF;
		}
		tone[RINGBACK_CNT-1].off_msec = RINGBACK_INTERVAL;

		pjmedia_tonegen_play(app_config.ringback_port, RINGBACK_CNT, tone,
					 PJMEDIA_TONEGEN_LOOP);


		status = pjsua_conf_add_port(app_config.pool, app_config.ringback_port,
						 &app_config.ringback_slot);
		if (status != PJ_SUCCESS)
			goto on_error;

		/* Ring (to alert incoming call) */
		name = pj_str((char *)"ring");
		status = pjmedia_tonegen_create2(app_config.pool, &name, 
						 app_config.media_cfg.clock_rate,
						 app_config.media_cfg.channel_count, 
						 samples_per_frame,
						 16, PJMEDIA_TONEGEN_LOOP, 
						 &app_config.ring_port);
		if (status != PJ_SUCCESS)
			goto on_error;

		for (i=0; i<RING_CNT; ++i) {
			tone[i].freq1 = RING_FREQ1;
			tone[i].freq2 = RING_FREQ2;
			tone[i].on_msec = RING_ON;
			tone[i].off_msec = RING_OFF;
		}
		tone[RING_CNT-1].off_msec = RING_INTERVAL;

		pjmedia_tonegen_play(app_config.ring_port, RING_CNT, 
					 tone, PJMEDIA_TONEGEN_LOOP);

		status = pjsua_conf_add_port(app_config.pool, app_config.ring_port,
						 &app_config.ring_slot);
		if (status != PJ_SUCCESS)
			goto on_error;

    }

	std::cout << "Inside on_pjsip_dll_init - 5" << std::endl;
    /* Create AVI player virtual devices */
    if (app_config.avi_cnt) {
#if PJMEDIA_HAS_VIDEO && PJMEDIA_VIDEO_DEV_HAS_AVI
		pjmedia_vid_dev_factory *avi_factory;

		status = pjmedia_avi_dev_create_factory(pjsua_get_pool_factory(),
												app_config.avi_cnt,
												&avi_factory);
		if (status != PJ_SUCCESS) {
			PJ_PERROR(1,(THIS_FILE, status, "Error creating AVI factory"));
			goto on_error;
		}

		for (i=0; i<app_config.avi_cnt; ++i) {
			pjmedia_avi_dev_param avdp;
			pjmedia_vid_dev_index avid;
			unsigned strm_idx, strm_cnt;

			app_config.avi[i].dev_id = PJMEDIA_VID_INVALID_DEV;
			app_config.avi[i].slot = PJSUA_INVALID_ID;

			pjmedia_avi_dev_param_default(&avdp);
			avdp.path = app_config.avi[i].path;

			status =  pjmedia_avi_dev_alloc(avi_factory, &avdp, &avid);
			if (status != PJ_SUCCESS) {
				PJ_PERROR(1,(THIS_FILE, status,
						 "Error creating AVI player for %.*s",
						 (int)avdp.path.slen, avdp.path.ptr));
				goto on_error;
			}

			PJ_LOG(4,(THIS_FILE, "AVI player %.*s created, dev_id=%d",
				  (int)avdp.title.slen, avdp.title.ptr, avid));

			app_config.avi[i].dev_id = avid;
			if (app_config.avi_def_idx == PJSUA_INVALID_ID)
			app_config.avi_def_idx = i;

			strm_cnt = pjmedia_avi_streams_get_num_streams(avdp.avi_streams);
			for (strm_idx=0; strm_idx<strm_cnt; ++strm_idx) {
				pjmedia_port *aud;
				pjmedia_format *fmt;
				pjsua_conf_port_id slot;
				char fmt_name[5];

				aud = pjmedia_avi_streams_get_stream(avdp.avi_streams,
													 strm_idx);
				fmt = &aud->info.fmt;

				pjmedia_fourcc_name(fmt->id, fmt_name);

				if (fmt->id == PJMEDIA_FORMAT_PCM) {
					status = pjsua_conf_add_port(app_config.pool, aud,
												 &slot);
					if (status == PJ_SUCCESS) {
					PJ_LOG(4,(THIS_FILE,
						  "AVI %.*s: audio added to slot %d",
						  (int)avdp.title.slen, avdp.title.ptr,
						  slot));
					app_config.avi[i].slot = slot;
				}
			} else {
				PJ_LOG(4,(THIS_FILE,
					  "AVI %.*s: audio ignored, format=%s",
					  (int)avdp.title.slen, avdp.title.ptr,
					  fmt_name));
			}
	    }
	}
#else
	PJ_LOG(2,(THIS_FILE,
		  "Warning: --play-avi is ignored because AVI is disabled"));
#endif	/* PJMEDIA_VIDEO_DEV_HAS_AVI */
    }

    /* Add UDP transport unless it's disabled. */
    if (!app_config.no_udp) {
		pjsua_acc_id aid;
		pjsip_transport_type_e type = PJSIP_TRANSPORT_UDP;
		
		status = pjsua_transport_create(type,
					&app_config.udp_cfg,
					&transport_id);
		if (status != PJ_SUCCESS)
			goto on_error;
		
		/* Add local account */
		pjsua_acc_add_local(transport_id, PJ_TRUE, &aid);
		if (PJMEDIA_HAS_VIDEO) {
			pjsua_acc_config acc_cfg;
			pjsua_acc_get_config(aid, &acc_cfg);
			app_config_init_video(&acc_cfg);
			pjsua_acc_modify(aid, &acc_cfg);
		}
		//pjsua_acc_set_transport(aid, transport_id);
		
		pjsua_acc_set_online_status(current_acc, PJ_TRUE);
		if (app_config.udp_cfg.port == 0) {
			pjsua_transport_info ti;
			pj_sockaddr_in *a;

			pjsua_transport_get_info(transport_id, &ti);
			a = (pj_sockaddr_in*)&ti.local_addr;

			tcp_cfg.port = pj_ntohs(a->sin_port);
		}
    }
	
	std::cout << "Inside on_pjsip_dll_init - 6" << std::endl;
    /* Add UDP IPv6 transport unless it's disabled. */
    if (!app_config.no_udp && app_config.ipv6) {
		pjsua_acc_id aid;
		pjsip_transport_type_e type = PJSIP_TRANSPORT_UDP6;
		pjsua_transport_config udp_cfg;

		udp_cfg = app_config.udp_cfg;
		if (udp_cfg.port == 0)
			udp_cfg.port = 5060;
		else
			udp_cfg.port += 10;
		status = pjsua_transport_create(type,
						&udp_cfg,
						&transport_id);
		if (status != PJ_SUCCESS)
			goto on_error;

		/* Add local account */
		pjsua_acc_add_local(transport_id, PJ_TRUE, &aid);
		if (PJMEDIA_HAS_VIDEO) {
			pjsua_acc_config acc_cfg;
			pjsua_acc_get_config(aid, &acc_cfg);
			app_config_init_video(&acc_cfg);
			if (app_config.ipv6)
				acc_cfg.ipv6_media_use = PJSUA_IPV6_ENABLED;
			pjsua_acc_modify(aid, &acc_cfg);
		}
		//pjsua_acc_set_transport(aid, transport_id);
		pjsua_acc_set_online_status(current_acc, PJ_TRUE);

		if (app_config.udp_cfg.port == 0) {
			pjsua_transport_info ti;
			pj_sockaddr_in *a;

			pjsua_transport_get_info(transport_id, &ti);
			a = (pj_sockaddr_in*)&ti.local_addr;

			tcp_cfg.port = pj_ntohs(a->sin_port);
		}
    }
	
    /* Add TCP transport unless it's disabled */
    if (!app_config.no_tcp) {

		pjsua_acc_id aid;

		status = pjsua_transport_create(PJSIP_TRANSPORT_TCP,
						&tcp_cfg, 
						&transport_id);
		if (status != PJ_SUCCESS)
			goto on_error;

		/* Add local account */
		pjsua_acc_add_local(transport_id, PJ_TRUE, &aid);
		if (PJMEDIA_HAS_VIDEO) {
			pjsua_acc_config acc_cfg;
			pjsua_acc_get_config(aid, &acc_cfg);
			app_config_init_video(&acc_cfg);
			pjsua_acc_modify(aid, &acc_cfg);
		}
		pjsua_acc_set_online_status(current_acc, PJ_TRUE);

    }

    /* Add TCP IPv6 transport unless it's disabled. */
    if (!app_config.no_tcp && app_config.ipv6) {
		pjsua_acc_id aid;
		pjsip_transport_type_e type = PJSIP_TRANSPORT_TCP6;

		tcp_cfg.port += 10;

		status = pjsua_transport_create(type,
						&tcp_cfg,
						&transport_id);
		if (status != PJ_SUCCESS)
			goto on_error;

		/* Add local account */
		pjsua_acc_add_local(transport_id, PJ_TRUE, &aid);
		if (PJMEDIA_HAS_VIDEO) {
			pjsua_acc_config acc_cfg;
			pjsua_acc_get_config(aid, &acc_cfg);
			app_config_init_video(&acc_cfg);
			if (app_config.ipv6)
			acc_cfg.ipv6_media_use = PJSUA_IPV6_ENABLED;
			pjsua_acc_modify(aid, &acc_cfg);
		}
		//pjsua_acc_set_transport(aid, transport_id);
		pjsua_acc_set_online_status(current_acc, PJ_TRUE);
    }

	std::cout << "Inside on_pjsip_dll_init - 7" << std::endl;

#if defined(PJSIP_HAS_TLS_TRANSPORT) && PJSIP_HAS_TLS_TRANSPORT!=0
    /* Add TLS transport when application wants one */
    if (app_config.use_tls) {

	pjsua_acc_id acc_id;

	/* Copy the QoS settings */
	tcp_cfg.tls_setting.qos_type = tcp_cfg.qos_type;
	pj_memcpy(&tcp_cfg.tls_setting.qos_params, &tcp_cfg.qos_params, 
		  sizeof(tcp_cfg.qos_params));

	/* Set TLS port as TCP port+1 */
	tcp_cfg.port++;
	status = pjsua_transport_create(PJSIP_TRANSPORT_TLS,
					&tcp_cfg, 
					&transport_id);
	tcp_cfg.port--;
	if (status != PJ_SUCCESS)
	    goto on_error;
	
	/* Add local account */
	pjsua_acc_add_local(transport_id, PJ_FALSE, &acc_id);
	if (PJMEDIA_HAS_VIDEO) {
	    pjsua_acc_config acc_cfg;
	    pjsua_acc_get_config(acc_id, &acc_cfg);
	    app_config_init_video(&acc_cfg);
	    pjsua_acc_modify(acc_id, &acc_cfg);
	}
	pjsua_acc_set_online_status(acc_id, PJ_TRUE);
    }

    /* Add TLS IPv6 transport unless it's disabled. */
    if (app_config.use_tls && app_config.ipv6) {
	pjsua_acc_id aid;
	pjsip_transport_type_e type = PJSIP_TRANSPORT_TLS6;

	tcp_cfg.port += 10;

	status = pjsua_transport_create(type,
					&tcp_cfg,
					&transport_id);
	if (status != PJ_SUCCESS)
	    goto on_error;

	/* Add local account */
	pjsua_acc_add_local(transport_id, PJ_TRUE, &aid);
	if (PJMEDIA_HAS_VIDEO) {
	    pjsua_acc_config acc_cfg;
	    pjsua_acc_get_config(aid, &acc_cfg);
	    app_config_init_video(&acc_cfg);
	    if (app_config.ipv6)
		acc_cfg.ipv6_media_use = PJSUA_IPV6_ENABLED;
	    pjsua_acc_modify(aid, &acc_cfg);
	}
	//pjsua_acc_set_transport(aid, transport_id);
	pjsua_acc_set_online_status(current_acc, PJ_TRUE);
    }

#endif

    if (transport_id == -1) {
		PJ_LOG(1,(THIS_FILE, "Error: no transport is configured"));
		status = -1;
		goto on_error;
    }


    /* Add accounts */
    for (i=0; i<app_config.acc_cnt; ++i) {
		app_config.acc_cfg[i].rtp_cfg = app_config.rtp_cfg;
		app_config.acc_cfg[i].reg_retry_interval = 300;
		app_config.acc_cfg[i].reg_first_retry_interval = 60;

		app_config_init_video(&app_config.acc_cfg[i]);

		status = pjsua_acc_add(&app_config.acc_cfg[i], PJ_TRUE, NULL);
		if (status != PJ_SUCCESS)
			goto on_error;
		pjsua_acc_set_online_status(current_acc, PJ_TRUE);
		}

    /* Add buddies */
    for (i=0; i<app_config.buddy_cnt; ++i) {
		status = pjsua_buddy_add(&app_config.buddy_cfg[i], NULL);
		if (status != PJ_SUCCESS) {
			PJ_PERROR(1,(THIS_FILE, status, "Error adding buddy"));
			goto on_error;
		}
    }

    /* Optionally disable some codec */
    for (i=0; i<app_config.codec_dis_cnt; ++i) {
		pjsua_codec_set_priority(&app_config.codec_dis[i],PJMEDIA_CODEC_PRIO_DISABLED);
#if PJSUA_HAS_VIDEO
		pjsua_vid_codec_set_priority(&app_config.codec_dis[i],PJMEDIA_CODEC_PRIO_DISABLED);
#endif
    }

    /* Optionally set codec orders */
    for (i=0; i<app_config.codec_cnt; ++i) {
		pjsua_codec_set_priority(&app_config.codec_arg[i],
				 (pj_uint8_t)(PJMEDIA_CODEC_PRIO_NORMAL+i+9));
#if PJSUA_HAS_VIDEO
		pjsua_vid_codec_set_priority(&app_config.codec_arg[i],
				     (pj_uint8_t)(PJMEDIA_CODEC_PRIO_NORMAL+i+9));
#endif
    }
    /* Use null sound device? */
#ifndef STEREO_DEMO
    if (app_config.null_audio) {
		status = pjsua_set_null_snd_dev();
		if (status != PJ_SUCCESS)
			return status;
    }
#endif

    if (app_config.capture_dev  != PJSUA_INVALID_ID ||
        app_config.playback_dev != PJSUA_INVALID_ID) 
    {
	status = pjsua_set_snd_dev(app_config.capture_dev, 
				   app_config.playback_dev);
	if (status != PJ_SUCCESS)
	    goto on_error;
    }
   
	std::cout << "Inside on_pjsip_dll_init - 8a" << std::endl;
	/* Initialization is done, now start pjsua */
    status = pjsua_start();
    if (status != PJ_SUCCESS) error_exit("Error starting pjsua", status);
	
	std::cout << "Inside on_pjsip_dll_init - 8" << std::endl;
	int dev_count;
	pjmedia_aud_dev_index dev_idx;
	dev_count = pjmedia_aud_dev_count();
	printf("Got %d audio devices\n", dev_count);
	for (dev_idx=0; dev_idx<dev_count; ++dev_idx) {
		pjmedia_aud_dev_info info;
		pjmedia_aud_dev_get_info(dev_idx, &info);
		printf("%d. %s (in=%d, out=%d)\n",
			dev_idx, info.name,
			info.input_count, info.output_count);
	}
	return PJ_SUCCESS;
on_error:
	app_destroy();
	return status;

	return 0;
}

int CPJSIPDll::on_pjsip_dll_add_account()
{
	std::cout << "Inside on_pjsip_dll_add_account" << std::endl;
	char realm[80];
	strcpy(realm, "*");
	
	pjsua_acc_config_default(&acc_cfg);
	pjsua_call_setting_default(&call_opt);
	call_opt.aud_cnt = app_config.aud_cnt;
    call_opt.vid_cnt = app_config.vid.vid_cnt;
	
	acc_cfg.id = pj_str((char *)cbParams->sipurl);
	acc_cfg.reg_uri = pj_str((char *)cbParams->sipProxyAddress);
	acc_cfg.cred_count = 1;
	acc_cfg.cred_info[0].scheme = pj_str((char *)"Digest");
	acc_cfg.cred_info[0].realm = pj_str((char *)"*");
	acc_cfg.cred_info[0].username = pj_str((char *)cbParams->userName);
	acc_cfg.cred_info[0].data_type = 0;
	acc_cfg.cred_info[0].data = pj_str((char *)cbParams->password);
	acc_cfg.proxy_cnt=0;
	acc_cfg.proxy[acc_cfg.proxy_cnt++] = pj_str((char *)cbParams->sipProxyAddress);

	acc_cfg.rtp_cfg = app_config.rtp_cfg;
	acc_cfg.sip_stun_use = PJSUA_STUN_USE_DISABLED;
	acc_cfg.media_stun_use = PJSUA_STUN_USE_DISABLED;
	acc_cfg.allow_contact_rewrite = 0;
	acc_cfg.use_rfc5626 = PJ_TRUE;

	app_config_init_video(&acc_cfg);
	
	pjsua_acc_id acc_id;
	status = pjsua_acc_add(&acc_cfg, PJ_TRUE, &acc_id);
	if (status != PJ_SUCCESS) {
	    pjsua_perror(THIS_FILE, "Error adding new account", status);
	}
	std::cout << "Added new account, id: " << acc_id << std::endl;
	{
		char event_log[100];
		pj_memset(event_log, 0, sizeof(event_log));
		sprintf(event_log,  "Added account %d", acc_id);
		pj_dll_event_write(4, (const char *)event_log, strlen(event_log));
	}
#if defined(PJSUA_HAS_VIDEO) && (PJSUA_HAS_VIDEO == 1)
	// enable video set it to AUTOTX and AUTORX
	unsigned int action = VID_ENABLE | VID_ACC_AUTORX_ON | VID_ACC_AUTOTX_ON;
	vid_handle_menu(action, NULL);

#endif
	/* Update call setting */
	update_call_setting();

	pjsua_msg_data_init(&msg_data);
	// TEST_MULTIPART(&msg_data);

	int timeout = 200;
	status = pjsua_handle_events(timeout);
	if (0 > status)
	{
			PJ_LOG(1,(THIS_FILE, "Error handling events!"));
	}
	return 0;
}

int CPJSIPDll::on_pjsip_dll_make_call()
{
	char event_log[100];
	pj_str_t tmp = pj_str((char *)cbParams->calleeURI);
	
	pj_memset(event_log, 0, sizeof(event_log));
	sprintf(event_log,  "Calling %s", cbParams->calleeURI);
	pj_dll_event_write(4, (const char *)event_log, strlen(event_log));
    std::cout << "Inside on_pjsip_dll_make_call: calling " << cbParams->calleeURI << std::endl;
    
	//pjsua_msg_data_init(&msg_data);
	// TEST_MULTIPART(&msg_data);
	//qosAnalyzer->reset_qos();
	pjsua_call_make_call( current_acc, &tmp, &call_opt, NULL, &msg_data, &call_id);
	cbParams->releaseCallId = call_id;
	update_call_setting();

	return 0;
}

int CPJSIPDll::on_pjsip_dll_answer_call()
{
	std::cout << "Inside on_pjsip_dll_answer_call" << std::endl;
	return 0;
}

int CPJSIPDll::on_pjsip_dll_release_call()
{
	char event_log[100];

	if ((current_call == -1) || (!pjsua_call_is_active(current_call))) {
			return 0;
	}
	pj_memset(event_log, 0, sizeof(event_log));
	sprintf(event_log,  "Release call %d", current_call);
	pj_dll_event_write(4, (const char *)event_log, strlen(event_log));

	std::cout << "Inside on_pjsip_dll_release_call, Id: " << cbParams->releaseCallId << std::endl;
	pjsua_call_hangup(current_call, 0, NULL, NULL);
	/* Update call setting */
	update_call_setting();
	
	
	return 0;
}

int CPJSIPDll::on_pjsip_dll_poll_for_events()
{
	status = !PJ_SUCCESS;
	//std::cout << "Inside on_pjsip_dll_poll_for_events" << std::endl;
	status = pjsua_handle_events(cbParams->timeout);
	if (0 > status)
	{
			//PJ_LOG(1,(THIS_FILE, "Error handling events!"));
	}
	
	return status;
}

int CPJSIPDll::on_pjsip_dll_get_params()
{
	return 0;
}

int CPJSIPDll::on_pjsip_dll_shutdown()
{
	char event_log[100];
	pj_memset(event_log, 0, sizeof(event_log));
	sprintf(event_log,  "Shutting Down");
	pj_dll_event_write(4, (const char *)event_log, strlen(event_log));
	std::cout << "Inside on_pjsip_dll_shutdown" << std::endl;
	
	//on_pj_write_cb = NULL;
	app_exiting = 1;
	app_destroy();
	
	//pj_thread_destroy(thread);   
	return 0;
}

extern "C" PJSIPDLL_API int DLLInvokeCallback(int cbNum, CallbackParams *cb)
{
	if(cbNum == pjsip_dll_init) {
		pjsipDll = new CPJSIPDll;
	}
	if(cbNum == pjsip_dll_shutdown) {
		pjsipDll->InvokeCallback(cbNum)();
		pjsipDll->setCBParams(NULL);
		delete pjsipDll;
		return 0;
	}
	if(cbNum == pjsip_dll_get_params) {
		pjsipDll->getCBParams(cb);
	}
	else {
		pjsipDll->setCBParams(cb);
	}
	return (pjsipDll->InvokeCallback(cbNum))();
}

// This is the constructor of a class that has been exported.
// see PJSIPDll.h for the class definition
CPJSIPDll::CPJSIPDll()
{
	std::cout << "Adding callbacks" << std::endl;
	// add call backs
	ADD_CB(pjsip_dll_init);
	ADD_CB(pjsip_dll_add_account);
	ADD_CB(pjsip_dll_make_call);
	ADD_CB(pjsip_dll_answer_call);
	ADD_CB(pjsip_dll_release_call);
	ADD_CB(pjsip_dll_poll_for_events);
	ADD_CB(pjsip_dll_get_params);
	ADD_CB(pjsip_dll_shutdown);

	/* The module instance. */
	memset(&mod_default_handler, 0, sizeof(mod_default_handler));
	mod_default_handler.name = pj_str((char *)"mod-default-handler");
	mod_default_handler.id = -1;
	//mod_default_handler.on_rx_request = &default_mod_on_rx_request;
	set_default_handler_on_rx_request(std::bind(&CPJSIPDll::default_mod_on_rx_request, this, _1));
	mod_default_handler.on_rx_request = &CPJSIPDll::forward_default_on_rx_request;
	pj_bzero(&app_config, sizeof(app_config));
	log_cb = NULL;
	thread = NULL;
	current_call = PJSUA_INVALID_ID;
	app_exiting = 0;
	return;
}

CPJSIPDll::~CPJSIPDll()
{
}
#if 0
extern "C" PJSIPDLL_API void DLLAttachPJLOGCallback(DumpPJLOG cb)
{
	std::cout << "Inside DLLAttachPJLOGCallback.." << std::endl;
	on_pj_write_cb = cb;
}

extern "C" PJSIPDLL_API void DLLAttachEventCallback(DumpPJLOG cb)
{
	std::cout << "Inside DLLAttachPJLOGCallback.." << std::endl;
	on_event_cb = cb;
}
#endif

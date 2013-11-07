// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the PJSIPDLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// PJSIPDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#include <pjsua-lib/pjsua.h>
#include <iostream>
#include <map>
#include <functional>

using namespace std::placeholders;

#define PJSIPDLL_API 

#define MAX_PARAM_SZ 80
//#define STEREO_DEMO
//#define TRANSPORT_ADAPTER_SAMPLE
//#define HAVE_MULTIPART_TEST

/* Ringtones		    US	       UK  */
#define RINGBACK_FREQ1	    440	    /* 400 */
#define RINGBACK_FREQ2	    480	    /* 450 */
#define RINGBACK_ON	    2000    /* 400 */
#define RINGBACK_OFF	    4000    /* 200 */
#define RINGBACK_CNT	    1	    /* 2   */
#define RINGBACK_INTERVAL   4000    /* 2000 */

#define RING_FREQ1	    800
#define RING_FREQ2	    640
#define RING_ON		    200
#define RING_OFF	    100
#define RING_CNT	    3
#define RING_INTERVAL	    3000

#define MAX_AVI             4

enum {
	pjsip_dll_init=0,
	pjsip_dll_add_account,
	pjsip_dll_make_call,
	pjsip_dll_answer_call,
	pjsip_dll_release_call,
	pjsip_dll_poll_for_events,
	pjsip_dll_get_params,
	pjsip_dll_shutdown,
	PJSIP_DLL_EVENTS_END
};

enum _videoRes_e {
	RES_CIF=0, // 352 x 288
	RES_QCIF, // 176 x 144
	RES_QVGA, // 320 x 240
	RES_IOSMEDIUM, // 360 x 480
	RES_VGA, // 640 x 480
	RES_4CIF, // 704 x 576
	RES_SVGA // 800 x 600
} videoRes_e;

typedef enum _AudioCodecs_e {
	AUCODEC_SPEEX_32000=0,
	AUCODEC_SPEEX_16000,
	AUCODEC_SPEEX_8000,
	AUCODEC_GSM,
	AUCODEC_PCMU,
	AUCODEC_PCMA,
	AUCODEC_ILBC,
	AUCODEC_MAX
} AudioCodecs_e;

typedef enum _VideoCodecs_e {
	VIDCODEC_H263=0,
	VIDCODEC_H263_1998,
	VIDCODEC_H264,
	VIDCODEC_MAX
} VideoCodecs_e;

enum {
	VID_ENABLE = (1 << 1),
	VID_DISABLE = (1 << 1),
	VID_ACC_AUTORX_ON = (1 << 2),
	VID_ACC_AUTORX_OFF = (1 << 2),
	VID_ACC_AUTOTX_ON = (1 << 3),
	VID_ACC_AUTOTX_OFF = (1 << 3),
	VID_SET_CAP_DEV = (1 << 4),
	VID_SET_REND_DEV = (1 << 5)
};

enum {
    CMD_REGISTER=0,
	CMD_CALL,
	RES_SUCCESS,
	RES_CONFIRMED,
	CMD_NONE
};

/* Call specific data */
struct call_data
{
    pj_timer_entry	    timer;
    pj_bool_t		    ringback_on;
    pj_bool_t		    ring_on;
};

/* Video settings */
struct app_vid
{
    unsigned		vid_cnt;
    int			    vcapture_dev;
    int			    vrender_dev;
    pj_bool_t		in_auto_show;
    pj_bool_t		out_auto_transmit;
};

/* Pjsua application data */
struct app_config_s
{
    pjsua_config	    cfg;
    pjsua_logging_config    log_cfg;
    pjsua_media_config	    media_cfg;
    pj_bool_t		    no_refersub;
    pj_bool_t		    ipv6;
    pj_bool_t		    enable_qos;
    pj_bool_t		    no_tcp;
    pj_bool_t		    no_udp;
    pj_bool_t		    use_tls;
    pjsua_transport_config  udp_cfg;
    pjsua_transport_config  rtp_cfg;
    pjsip_redirect_op	    redir_op;

    unsigned		    acc_cnt;
    pjsua_acc_config	    acc_cfg[PJSUA_MAX_ACC];

    unsigned		    buddy_cnt;
    pjsua_buddy_config	    buddy_cfg[PJSUA_MAX_BUDDIES];

    struct call_data	    call_data[PJSUA_MAX_CALLS];

    pj_pool_t		   *pool;
    /* Compatibility with older pjsua */

    unsigned		    codec_cnt;
    pj_str_t		    codec_arg[32];
    unsigned		    codec_dis_cnt;
    pj_str_t                codec_dis[32];
    pj_bool_t		    null_audio;
    unsigned		    wav_count;
    pj_str_t		    wav_files[32];
    unsigned		    tone_count;
    pjmedia_tone_desc	    tones[32];
    pjsua_conf_port_id	    tone_slots[32];
    pjsua_player_id	    wav_id;
    pjsua_conf_port_id	    wav_port;
    pj_bool_t		    auto_play;
    pj_bool_t		    auto_play_hangup;
    pj_timer_entry	    auto_hangup_timer;
    pj_bool_t		    auto_loop;
    pj_bool_t		    auto_conf;
    pj_str_t		    rec_file;
    pj_bool_t		    auto_rec;
    pjsua_recorder_id	    rec_id;
    pjsua_conf_port_id	    rec_port;
    unsigned		    auto_answer;
    unsigned		    duration;

#ifdef STEREO_DEMO
    pjmedia_snd_port	   *snd;
    pjmedia_port	   *sc, *sc_ch1;
    pjsua_conf_port_id	    sc_ch1_slot;
#endif

    float		    mic_level,
			    speaker_level;

    int			    capture_dev, playback_dev;
    unsigned		capture_lat, playback_lat;

    pj_bool_t		no_tones;
    int			    ringback_slot;
    int			    ringback_cnt;
    pjmedia_port	*ringback_port;
    int			    ring_slot;
    int			    ring_cnt;
    pjmedia_port	*ring_port;

    struct app_vid	vid;
    unsigned		aud_cnt;

    /* AVI to play */
    unsigned        avi_cnt;
    struct {
	pj_str_t		path;
	pjmedia_vid_dev_index	dev_id;
	pjsua_conf_port_id	slot;
    } avi[MAX_AVI];
    pj_bool_t               avi_auto_play;
    int			    avi_def_idx;

};

//static pjsua_acc_id	current_acc;
#define current_acc	pjsua_acc_get_default()

#if defined(PJMEDIA_HAS_RTCP_XR) && (PJMEDIA_HAS_RTCP_XR != 0)
#   define SOME_BUF_SIZE	(1024 * 10)
#else
#   define SOME_BUF_SIZE	(1024 * 3)
#endif

static char some_buf[SOME_BUF_SIZE];

#ifdef STEREO_DEMO
static void stereo_demo();
#endif

typedef struct CallbackParams_s {
	char displayName[80];
	char userName[80];
	char sipIdentity[80];
	char sipProxyAddress[80];
	char password[80];
	char sipurl[80];
	char calleeURI[80];

	unsigned int releaseCallId;
	unsigned int answerCallId;
	unsigned int answerCode;
	unsigned int timeout;
	unsigned int rate_percent;
	
    unsigned int tcpOnly;
	unsigned int sipPort;
	unsigned int cmd;
	unsigned int res;

} CallbackParams;

//#define ADD_CB(level) m_callbacks_from_gui[level] = std::bind(&CPJSIPDll::on_ ## level, *this, _1)
//typedef std::function<int (CallbackParams *)> PJSIPDLLCB;

#define ADD_CB(level) m_callbacks_from_gui[level] = std::bind(&CPJSIPDll::on_ ## level, this)
typedef std::function<int ()> PJSIPDLLCB;

// This class is exported from the PJSIPDll.dll
class CPJSIPDll {
public:
	CPJSIPDll(void);
	virtual ~CPJSIPDll(void);
	// TODO: add your methods here.
	virtual void setCBParams(CallbackParams *cb) { cbParams = cb; };
	virtual void getCBParams(CallbackParams *cb) { cb->res = cbParams->res; };

	// WPF GUI RELATED
	virtual int on_pjsip_dll_init();
	virtual int on_pjsip_dll_add_account();
	virtual int on_pjsip_dll_make_call();
	virtual int on_pjsip_dll_answer_call();
	virtual int on_pjsip_dll_release_call();
	virtual int on_pjsip_dll_poll_for_events();
	virtual int on_pjsip_dll_get_params();
	virtual int on_pjsip_dll_shutdown();
	virtual PJSIPDLLCB InvokeCallback(int n) { return m_callbacks_from_gui[n]; }
	
	pj_bool_t default_mod_on_rx_request(pjsip_rx_data *rdata);
	pj_bool_t isAppExiting() { return app_exiting; };

	// use the following functions for callback for mod_default_handler
	pjsip_module mod_default_handler;
	
	// Adaptive bit rate callback
	// Adaptive Bit rate callback

	static pj_bool_t forward_default_on_rx_request(pjsip_rx_data *rdata) { return s_func_on_rx_request(rdata); }
	static void forward_call_timeout_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry) { return s_func_call_timeout_callback(timer_heap, entry); }
	static void forward_on_call_state(pjsua_call_id call_id, pjsip_event *e) { return s_func_on_call_state(call_id, e); }
	static void forward_on_call_tsx_state(pjsua_call_id call_id, pjsip_transaction *tsx, pjsip_event *e) { return s_func_on_call_tsx_state(call_id, tsx, e); }
	static void forward_on_call_generic_media_state(pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error) { return s_func_on_call_generic_media_state(ci, mi, has_error); }
	static void forward_on_call_audio_state(pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error) { return s_func_on_call_audio_state(ci, mi, has_error); }
	static void forward_on_call_video_state(pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error) { return s_func_on_call_video_state(ci, mi, has_error); }
	static void forward_on_call_media_state(pjsua_call_id call_id) { return s_func_on_call_media_state(call_id); }
	static void forward_call_on_dtmf_callback(pjsua_call_id call_id, int dtmf) { return s_func_call_on_dtmf_callback(call_id, dtmf); }
	static void forward_on_reg_state(pjsua_acc_id acc_id) { return s_func_on_reg_state(acc_id); }
	static void forward_on_nat_detect(const pj_stun_nat_detect_result *res) { return s_func_on_nat_detect(res); }
	static void forward_on_mwi_info(pjsua_acc_id acc_id, pjsua_mwi_info *mwi_info) { return s_func_on_mwi_info(acc_id, mwi_info); }
	static void forward_on_transport_state(pjsip_transport *tp, pjsip_transport_state state, const pjsip_transport_state_info *info) { return s_func_on_transport_state(tp, state, info); }
	static void forward_on_ice_transport_error(int index, pj_ice_strans_op op, pj_status_t status, void *param) { return s_func_on_ice_transport_error(index,op, status, param); }
	static pj_status_t forward_on_snd_dev_operation(int operation) { return s_func_on_snd_dev_operation(operation); }
	static void forward_on_call_media_event(pjsua_call_id call_id, unsigned med_idx, pjmedia_event *event) { return s_func_on_call_media_event(call_id, med_idx, event); }
	static pj_status_t forward_on_playfile_done(pjmedia_port *port, void *usr_data) { return s_func_on_playfile_done(port, usr_data); }
	static void forward_hangup_timeout_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry) { return s_func_hangup_timeout_callback(timer_heap, entry); }

	// directly expose these for now since PJSUA library calls make use of these variables
	struct app_config_s app_config;
	pjsua_call_id	current_call;
	pjsua_call_setting call_opt;
	pjsua_acc_config acc_cfg;
	pj_thread_t* thread;
	pj_timer_entry qosTimer;

private:
	pj_status_t status;
	pj_log_func     *log_cb;
	std::map<int, PJSIPDLLCB> m_callbacks_from_gui;
	CallbackParams *cbParams;
	pj_bool_t 	app_restart;
	pj_bool_t app_exiting;
	pjsua_msg_data msg_data;
	pjsua_call_id call_id;
	pj_time_val delay;

	// PJSUA RELATED
	void ringback_start(pjsua_call_id call_id);
	void ring_start(pjsua_call_id call_id);
	void ring_stop(pjsua_call_id call_id);
	
	void default_config();
	void update_call_setting();

	// Adaptive Bit rate callback
	void adaptive_bitrate_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry);

	// PJSUA callbacks
	void call_timeout_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry);
	void on_call_state(pjsua_call_id call_id, pjsip_event *e);
	void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);
	void on_call_tsx_state(pjsua_call_id call_id, pjsip_transaction *tsx, pjsip_event *e);
	void on_call_generic_media_state(pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error);
	void on_call_audio_state(pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error);
	void arrange_window(pjsua_vid_win_id wid);
	void on_call_video_state(pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error);
	void on_call_media_state(pjsua_call_id call_id);
	void call_on_dtmf_callback(pjsua_call_id call_id, int dtmf);
	pjsip_redirect_op call_on_redirected(pjsua_call_id call_id, const pjsip_uri *target, const pjsip_event *e);
	void on_reg_state(pjsua_acc_id acc_id);
	void on_call_replaced(pjsua_call_id old_call_id, pjsua_call_id new_call_id);
	void on_nat_detect(const pj_stun_nat_detect_result *res);
	void on_mwi_info(pjsua_acc_id acc_id, pjsua_mwi_info *mwi_info);
	void on_transport_state(pjsip_transport *tp, pjsip_transport_state state,
			       const pjsip_transport_state_info *info);
	void on_ice_transport_error(int index, pj_ice_strans_op op, pj_status_t status, void *param);
	pj_status_t on_snd_dev_operation(int operation);
	void on_call_media_event(pjsua_call_id call_id, unsigned med_idx, pjmedia_event *event);
	pj_status_t on_playfile_done(pjmedia_port *port, void *usr_data);
	void hangup_timeout_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry);
	void conf_list(void);
	pj_status_t app_destroy(void);

	int my_atoi(const char *cs);
	void app_dump(pj_bool_t detail);
	void log_call_dump(int call_id) ;
	pj_bool_t find_next_call(void);
	pj_bool_t find_prev_call(void);
	void print_acc_status(int acc_id);
	void send_request(char *cstr_method, const pj_str_t *dst_uri);
	void vid_print_dev(int id, const pjmedia_vid_dev_info *vdi, const char *title);
	void vid_list_devs(void);
	void app_config_init_video(pjsua_acc_config *acc_cfg);
	void app_config_show_video(int acc_id, const pjsua_acc_config *acc_cfg);
	void vid_handle_menu(int action, char *param);
	
	//void vid_handle_menu(char *menuin);
	void error_exit(const char *title, pj_status_t status);
	void simple_registrar(pjsip_rx_data *rdata);
	
	// Adaptive bit rate callback
	static std::function<void (pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)> s_func_adaptive_bitrate_callback;
	static void set_adaptive_bitrate_callback(std::function<void (pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)> f) { s_func_adaptive_bitrate_callback = f; }

	// for library callbacks, initialize the static functor with the handler and call it from the forwarding function
	static std::function<pj_bool_t (pjsip_rx_data *rdata)> s_func_on_rx_request;
	static void set_default_handler_on_rx_request(std::function<pj_bool_t (pjsip_rx_data *rdata)> f) { s_func_on_rx_request = f; }

	static std::function<void (pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)> s_func_call_timeout_callback;
	static void set_call_timeout_callback(std::function<void (pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)> f) { s_func_call_timeout_callback = f; }

	static std::function<void (pjsua_call_id call_id, pjsip_event *e)> s_func_on_call_state;
	static void set_on_call_state(std::function<void (pjsua_call_id call_id, pjsip_event *e)> f) { s_func_on_call_state = f; }

	static std::function<void (pjsua_call_id call_id, pjsip_transaction *tsx, pjsip_event *e)> s_func_on_call_tsx_state;
	static void set_on_call_tsx_state(std::function<void (pjsua_call_id call_id, pjsip_transaction *tsx, pjsip_event *e)> f) { s_func_on_call_tsx_state = f; }

	static std::function<void (pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error)> s_func_on_call_generic_media_state;
	static void set_on_call_generic_media_state(std::function<void (pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error)> f) { s_func_on_call_generic_media_state = f; }

	static std::function<void (pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error)> s_func_on_call_audio_state;
	static void set_on_call_audio_state(std::function<void (pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error)> f) { s_func_on_call_audio_state = f; }

	static std::function<void (pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error)> s_func_on_call_video_state;
	static void set_on_call_video_state(std::function<void (pjsua_call_info *ci, unsigned mi, pj_bool_t *has_error)> f) { s_func_on_call_video_state = f; }

	static std::function<void (pjsua_call_id call_id)> s_func_on_call_media_state;
	static void set_on_call_media_state(std::function<void (pjsua_call_id call_id)> f) { s_func_on_call_media_state = f; }

	static std::function<void (pjsua_call_id call_id, int dtmf)> s_func_call_on_dtmf_callback;
	static void set_call_on_dtmf_callback(std::function<void (pjsua_call_id call_id, int dtmf)> f) { s_func_call_on_dtmf_callback = f; }

	static std::function<void (pjsua_acc_id acc_id)> s_func_on_reg_state;
	static void set_on_reg_state(std::function<void (pjsua_acc_id acc_id)> f) { s_func_on_reg_state = f; }

	static std::function<void (const pj_stun_nat_detect_result *res)> s_func_on_nat_detect;
	static void set_on_nat_detect(std::function<void (const pj_stun_nat_detect_result *res)> f) { s_func_on_nat_detect = f; }

	static std::function<void (pjsua_acc_id acc_id, pjsua_mwi_info *mwi_info)> s_func_on_mwi_info;
	static void set_on_mwi_info(std::function<void (pjsua_acc_id acc_id, pjsua_mwi_info *mwi_info)> f) { s_func_on_mwi_info = f; }

	static std::function<void (pjsip_transport *tp, pjsip_transport_state state, const pjsip_transport_state_info *info)> s_func_on_transport_state;
	static void set_on_transport_state(std::function<void (pjsip_transport *tp, pjsip_transport_state state, const pjsip_transport_state_info *info)> f) { s_func_on_transport_state = f; }

	static std::function<void (int index, pj_ice_strans_op op, pj_status_t status, void *param)> s_func_on_ice_transport_error;
	static void set_on_ice_transport_error(std::function<void (int index, pj_ice_strans_op op, pj_status_t status, void *param)> f) { s_func_on_ice_transport_error = f; }

	static std::function<pj_status_t (int operation)> s_func_on_snd_dev_operation;
	static void set_on_snd_dev_operation(std::function<pj_status_t (int operation)> f) { s_func_on_snd_dev_operation = f; }

	static std::function<void (pjsua_call_id call_id, unsigned med_idx, pjmedia_event *event)> s_func_on_call_media_event;
	static void set_on_call_media_event(std::function<void (pjsua_call_id call_id, unsigned med_idx, pjmedia_event *event)> f) { s_func_on_call_media_event = f; }

	static std::function<pj_status_t (pjmedia_port *port, void *usr_data)> s_func_on_playfile_done;
	static void set_on_playfile_done(std::function<pj_status_t (pjmedia_port *port, void *usr_data)> f) { s_func_on_playfile_done = f; }

	static std::function<void (pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)> s_func_hangup_timeout_callback;
	static void set_hangup_timeout_callback(std::function<void (pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)> f) { s_func_hangup_timeout_callback = f; }
};


// calback function definitions
//extern "C" PJSIPDLL_API int DLLInvokeCallback(int cbNum, CallbackParams *cb);
//extern "C" PJSIPDLL_API void DLLAttachPJLOGCallback(DumpPJLOG cb);

// IM & Presence api
// TBD


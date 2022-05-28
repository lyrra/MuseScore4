
#include "mux.h"
#include "muxlib.h"


namespace Ms {


/*
 * controller for muxseq client
 *
 */

static bool g_threads_started = false;

void mux_network_client_ctrl();
void mux_network_client_audio();

static std::vector<std::thread> muxThreads;

int g_ctrl_audio_error = 0;
int g_ctrl_audio_running = 0;

static struct Mux::MuxSocket g_ctrl_socket;
static struct Mux::MuxSocket g_audio_socket;

void mux_network_client_ctrl()
{
    Mux::mux_network_connect(g_ctrl_socket, "tcp://localhost:7770");
//    mux_network_mainloop_ctrl();
}

void mux_network_client_audio()
{
    Mux::mux_network_connect(g_audio_socket, "tcp://localhost:7771");
//    mux_network_mainloop_audio();
}

void mux_network_close_audio()
{
    // void mux_network_close(struct MuxSocket &sock)
        // zmq_close(zmq_socket_audio);
        // zmq_ctx_destroy(zmq_context_audio);
}
void mux_network_close_ctrl()
{
    // zmq_close(zmq_socket_ctrl);
    // zmq_ctx_destroy(zmq_context_ctrl);
}



/**/

void muxseq_ctrl_zmq_thread_init(std::string _notused)
{
    mux_network_client_ctrl();
}

void muxseq_audio_zmq_thread_init(std::string _notused)
{
    mux_network_client_audio();
}

void muxseq_start_threads()
{
    if (g_threads_started) return;
    g_threads_started = true;
    std::vector<std::thread> threadv;
    //
    std::thread zmqCtrlThread(muxseq_ctrl_zmq_thread_init, "notused");
    threadv.push_back(std::move(zmqCtrlThread));
    //
    std::thread zmqAudioThread(muxseq_audio_zmq_thread_init, "notused");
    threadv.push_back(std::move(zmqAudioThread));
    // move threads to heap
    muxThreads = std::move(threadv);
}

void mux_musescore_client_start() {
    muxseq_start_threads();
}


} // namespace Ms

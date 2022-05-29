
#include "mux.h"
#include "muxlib.h"

namespace Ms {

/*
 * controller for muxseq client
 *
 */

static bool g_threads_started = false;

static std::vector<std::thread> muxseq_Threads;

static struct Mux::MuxSocket g_muxseq_query_client_socket;
static struct Mux::MuxSocket g_muxseq_bulletin_client_socket;

/**/
// void mux_network_close(struct MuxSocket &sock)

void muxseq_query_client_thread_init(std::string _notused)
{
     Mux::mux_network_query_client(g_muxseq_query_client_socket, "MUX_MUSESCORE_QUERY_CLIENT_URL", true);
//    mux_network_mainloop_audio();
}

void muxseq_bulletin_client_thread_init(std::string _notused)
{
     Mux::mux_network_bulletin_client(g_muxseq_bulletin_client_socket, "MUX_MUSESCORE_BULLETIN_CLIENT_URL");
//    mux_network_mainloop_ctrl();
}

void mux_musescore_client_start()
{
    if (g_threads_started) {
        qWarning("musescore-mux-client already started");
        return;
    }
    g_threads_started = true;
    std::vector<std::thread> threadv;
    //
    std::thread zmqMuxseqQueryThread(muxseq_query_client_thread_init, "notused");
    threadv.push_back(std::move(zmqMuxseqQueryThread));
    //
    std::thread zmqMuxseqBulletinThread(muxseq_bulletin_client_thread_init, "notused");
    threadv.push_back(std::move(zmqMuxseqBulletinThread));
    // move threads to heap
    muxseq_Threads = std::move(threadv);
}


} // namespace Ms

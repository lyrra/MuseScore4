/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "jackmidiinport.h"

#include <jack/jack.h>
#include <jack/midiport.h>

#include "midierrors.h"
#include "stringutils.h"
#include "translation.h"
#include "defer.h"
#include "log.h"

struct mu::midi::JackMidiInPort::Jack {
    jack_port_t* midiIn = nullptr;
    int client = -1;
    int port = -1;
};

using namespace mu::midi;

void JackMidiInPort::init()
{
    m_jack = std::make_shared<Jack>();

    m_devicesListener.startWithCallback([this]() {
        return availableDevices();
    });

    m_devicesListener.devicesChanged().onNotify(this, [this]() {
        bool connectedDeviceRemoved = true;
        for (const MidiDevice& device: availableDevices()) {
            if (m_deviceID == device.id) {
                connectedDeviceRemoved = false;
            }
        }

        if (connectedDeviceRemoved) {
            disconnect();
        }

        m_availableDevicesChanged.notify();
    });
}

void JackMidiInPort::deinit()
{
    if (isConnected()) {
        disconnect();
    }
}

std::vector<MidiDevice> JackMidiInPort::availableDevices() const
{
    std::lock_guard lock(m_devicesMutex);

    //int streams = 0; //SND_SEQ_OPEN_INPUT;
    //unsigned int cap = 0; //SND_SEQ_PORT_CAP_SUBS_WRITE | SND_SEQ_PORT_CAP_WRITE;
    //unsigned int type = 0; //SND_SEQ_PORT_TYPE_PORT | SND_SEQ_PORT_TYPE_HARDWARE;

    std::vector<MidiDevice> ret;

    ret.push_back({ NONE_DEVICE_ID, trc("midi", "No device") });

    //snd_seq_client_info_t* cinfo;
    //snd_seq_port_info_t* pinfo;
    //int client;
    int err;
    //snd_seq_t* handle;

    err = 0; // snd_seq_open(&handle, "hw", streams, 0);
    if (err < 0) {
        /* Use snd_strerror(errno) to get the error here. */
        return ret;
    }

    //snd_seq_client_info_alloca(&cinfo);
    //snd_seq_client_info_set_client(cinfo, -1);
#if 0
    int index = 0;
    while (snd_seq_query_next_client(handle, cinfo) >= 0) {
        client = snd_seq_client_info_get_client(cinfo);
        if (client == SND_SEQ_CLIENT_SYSTEM) {
            continue;
        }

        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_client(pinfo, client);

        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(handle, pinfo) >= 0) {
            uint32_t types = snd_seq_port_info_get_type(pinfo);
            uint32_t caps = snd_seq_port_info_get_capability(pinfo);

            bool canConnect = ((caps & cap) == cap) && ((types & type) == type);

            if (canConnect) {
                MidiDevice dev;
                dev.name = snd_seq_client_info_get_name(cinfo);

                int client = snd_seq_port_info_get_client(pinfo);
                int port = snd_seq_port_info_get_port(pinfo);
                dev.id = makeUniqueDeviceId(index++, client, port);

                ret.push_back(std::move(dev));
            }
        }
    }
#endif

//    snd_seq_close(handle);

    return ret;
}

mu::async::Notification JackMidiInPort::availableDevicesChanged() const
{
    return m_availableDevicesChanged;
}

mu::Ret JackMidiInPort::connect(const MidiDeviceID& deviceID)
{
    if (!deviceExists(deviceID)) {
        return make_ret(Err::MidiFailedConnect, "not found device, id: " + deviceID);
    }

    if (isConnected()) {
        disconnect();
    }

    DEFER {
        m_deviceChanged.notify();
    };

    Ret ret = make_ok();

    if (!deviceID.empty() && deviceID != NONE_DEVICE_ID) {
        std::vector<int> deviceParams = splitDeviceId(deviceID);
        IF_ASSERT_FAILED(deviceParams.size() == 3) {
            return make_ret(Err::MidiInvalidDeviceID, "invalid device id: " + deviceID);
        }
#if 0
        int err = snd_seq_open(&m_jack->midiIn, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK);
        if (err < 0) {
            return make_ret(Err::MidiFailedConnect, "failed open seq, err: " + std::string(snd_strerror(err)));
        }

        snd_seq_set_client_name(m_jack->midiIn, "MuseScore");
        int port
            = snd_seq_create_simple_port(m_jack->midiIn, "MuseScore Input Port", SND_SEQ_PORT_CAP_WRITE, SND_SEQ_PORT_TYPE_MIDI_GENERIC);
        if (port < 0) {
            return make_ret(Err::MidiFailedConnect, "failed create port");
        }

        m_jack->client = deviceParams.at(1);
        m_jack->port = deviceParams.at(2);
        err = snd_seq_connect_from(m_jack->midiIn, 0, m_jack->client, m_jack->port);
        if (err < 0) {
            return make_ret(Err::MidiFailedConnect,  "failed connect, err: " + std::string(snd_strerror(err)));
        }

        m_deviceID = deviceID;
        ret = run();
#endif
    } else {
        m_deviceID = deviceID;
    }

    if (ret) {
        LOGD() << "Connected to " << m_deviceID;
    }

    return ret;
}

void JackMidiInPort::disconnect()
{
    if (!isConnected()) {
        return;
    }
#if 0
    snd_seq_disconnect_to(m_jack->midiIn, 0, m_jack->client, m_jack->port);
    snd_seq_close(m_jack->midiIn);

    stop();

    LOGD() << "Disconnected from " << m_deviceID;
#endif
    m_jack->client = -1;
    m_jack->port = -1;
    m_jack->midiIn = nullptr;
    m_deviceID.clear();
}

bool JackMidiInPort::isConnected() const
{
    return m_jack && m_jack->midiIn && !m_deviceID.empty();
}

MidiDeviceID JackMidiInPort::deviceID() const
{
    return m_deviceID;
}

mu::async::Notification JackMidiInPort::deviceChanged() const
{
    return m_deviceChanged;
}

mu::async::Channel<tick_t, Event> JackMidiInPort::eventReceived() const
{
    return m_eventReceived;
}

mu::Ret JackMidiInPort::run()
{
    if (!isConnected()) {
        return make_ret(Err::MidiNotConnected);
    }

    if (m_thread) {
        LOGW() << "already started";
        return Ret(true);
    }

    m_running.store(true);
    m_thread = std::make_shared<std::thread>(process, this);
    return Ret(true);
}

void JackMidiInPort::stop()
{
    if (!m_thread) {
        LOGW() << "already stopped";
        return;
    }

    m_running.store(false);
    m_thread->join();
    m_thread = nullptr;
}

void JackMidiInPort::process(JackMidiInPort* self)
{
    self->doProcess();
}

void JackMidiInPort::doProcess()
{
    //snd_seq_event_t* ev = nullptr;
    //uint32_t data = 0;
    //uint32_t value = 0;
    Event e;
#if 0
    while (m_running.load() && isConnected()) {
        snd_seq_event_input(m_jack->midiIn, &ev);

        auto sleep = []() {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        };

        if (!ev) {
            sleep();
            continue;
        }

        switch (ev->type) {
        case SND_SEQ_EVENT_SYSEX:
        {
            NOT_SUPPORTED << "event type: SND_SEQ_EVENT_SYSEX";
            continue;
        }
        case SND_SEQ_EVENT_NOTEOFF:
            data = 0x80
                   | (ev->data.note.channel & 0x0F)
                   | ((ev->data.note.note & 0x7F) << 8)
                   | ((ev->data.note.velocity & 0x7F) << 16);
            break;
        case SND_SEQ_EVENT_NOTEON:
            data = 0x90
                   | (ev->data.note.channel & 0x0F)
                   | ((ev->data.note.note & 0x7F) << 8)
                   | ((ev->data.note.velocity & 0x7F) << 16);
            break;
        case SND_SEQ_EVENT_KEYPRESS:
            data = 0xA0
                   | (ev->data.note.channel & 0x0F)
                   | ((ev->data.note.note & 0x7F) << 8)
                   | ((ev->data.note.velocity & 0x7F) << 16);
            break;
        case SND_SEQ_EVENT_CONTROLLER:
            data = 0xB0
                   | (ev->data.control.channel & 0x0F)
                   | ((ev->data.control.param & 0x7F) << 8)
                   | ((ev->data.control.value & 0x7F) << 16);
            break;
        case SND_SEQ_EVENT_PGMCHANGE:
            data = 0xC0
                   | (ev->data.control.channel & 0x0F)
                   | ((ev->data.control.value & 0x7F) << 8);
            break;
        case SND_SEQ_EVENT_CHANPRESS:
            data = 0xD0
                   | (ev->data.control.channel & 0x0F)
                   | ((ev->data.control.value & 0x7F) << 8);
            break;
        case SND_SEQ_EVENT_PITCHBEND:
            value = ev->data.control.value + 8192;
            data = 0xE0
                   | (ev->data.note.channel & 0x0F)
                   | ((value & 0x7F) << 8)
                   | (((value >> 7) & 0x7F) << 16);
            break;
        default:
            NOT_SUPPORTED << "event type: " << ev->type;
            continue;
        }

        e = Event::fromMIDI10Package(data);

        e = e.toMIDI20();
        if (e) {
            m_eventReceived.send(static_cast<tick_t>(ev->time.tick), e);
        }

        sleep();
    }
#endif
}

bool JackMidiInPort::deviceExists(const MidiDeviceID& deviceId) const
{
    for (const MidiDevice& device : availableDevices()) {
        if (device.id == deviceId) {
            return true;
        }
    }

    return false;
}

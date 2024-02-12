/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2023 MuseScore BVBA and others
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
#include "framework/audio/audiomodule.h"
#include "jackmidioutport.h"

#include <jack/jack.h>
#include <jack/midiport.h>

#include "midierrors.h"
#include "translation.h"
#include "log.h"

struct mu::midi::JackMidiOutPort::Jack {
    jack_port_t* midiOut = nullptr;
    jack_client_t* client = nullptr;
    int port = -1;
    int segmentSize;
};

using namespace mu::midi;

void JackMidiOutPort::preamble(std::shared_ptr<mu::audio::IAudioDriver> audioDriver)
{
    LOGI("---- linux JACK-midi output preamble ----");
    m_audioDriver = audioDriver;
    m_jack = std::make_unique<Jack>();
    // FIX: do we need the jack-client (handle)?
    //m_jack->client = static_cast<jack_client_t*>(jackClient);
}

void JackMidiOutPort::init()
{
}

void JackMidiOutPort::deinit()
{
    if (isConnected()) {
        disconnect();
    }
}

std::vector<MidiDevice> JackMidiOutPort::availableDevices() const
{
    //LOGI("---- linux JACK availableDevices ----");
    std::vector<MidiDevice> ret;
    MidiDevice dev;
    dev.name = "JACK";
    dev.id = makeUniqueDeviceId(0,9999,0);
    ret.push_back(std::move(dev));
    return ret;
}

mu::Ret JackMidiOutPort::connect(const MidiDeviceID& deviceID)
{
    LOGI("---- JACK output connect ----");
    if (!m_jack->client) {
        return make_ret(Err::MidiInvalidDeviceID, "jack-open fail, device: " + deviceID);
    }

    return Ret(true);
}

void JackMidiOutPort::disconnect()
{
    LOGI("---- linux JACK disconnect ----");
    if (!isConnected()) {
        return;
    }
    if (!m_jack->client) {
        return;
    }
    jack_client_t* client = m_jack->client;

    //FIX:
    //const char* sn = jack_port_name((jack_port_t*) src);
    //const char* dn = jack_port_name((jack_port_t*) dst);
    //jackdisconnect(m_jack->midiOut, 0, m_jack->client, m_jack->port);
    if (jack_deactivate(client)) {
        LOGE() << "failed to deactive jack";
    }
    m_jack->port = -1;
    m_jack->midiOut = nullptr;
    m_deviceID.clear();
}

bool JackMidiOutPort::isConnected() const
{
    LOGI("---- JACK output isConnect %lx, %lx, %s ----", m_jack.get(), m_jack->midiOut, (!m_deviceID.empty()) ? "empty" : "not empty");
    return !(m_jack.get() && m_jack->midiOut && !m_deviceID.empty());
}

MidiDeviceID JackMidiOutPort::deviceID() const
{
    LOGI("---- JACK output deviceID ----");
    return m_deviceID;
}

bool JackMidiOutPort::supportsMIDI20Output() const
{
    LOGI("---- JACK output supportsMIDI20Output ----");
    return false;
}

bool JackMidiOutPort::deviceExists(const MidiDeviceID& deviceId) const
{
    LOGI("---- JACK-midi output deviceExists ----");
    for (const MidiDevice& device : availableDevices()) {
        if (device.id == deviceId) {
            return true;
        }
    }

    return false;
}

// Not used
mu::Ret JackMidiOutPort::sendEvent(const Event&)
{
    return Ret(true);
}

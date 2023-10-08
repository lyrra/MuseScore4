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
//#include <thread>
//#include <queue>
//#include <mutex>
#include "linuxmidioutport.h"
#include "framework/audio/audiomodule.h"

#include "midierrors.h"
#include "translation.h"
#include "defer.h"
#include "log.h"

using namespace mu::midi;

void LinuxMidiOutPort::init(std::shared_ptr<mu::audio::AudioModule> am)
{
    LOGI("---- init ----");
    //m_alsa = std::make_shared<Alsa>();
    std::shared_ptr<mu::audio::IAudioDriver> audioDriver;

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    m_audioModule = am;
#endif

#if JACK_AUDIO
    m_midiOutPortJack = std::make_unique<JackMidiOutPort>();
    m_midiOutPortJack->preamble(audioDriver);
    m_midiOutPortJack->init();
#endif
    m_midiOutPortAlsa = std::make_unique<AlsaMidiOutPort>();
    m_midiOutPortAlsa->init();

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

void LinuxMidiOutPort::deinit()
{
    LOGI("---- linux deinit ----");
    if (isConnected()) {
        disconnect();
    }
}

std::vector<MidiDevice> LinuxMidiOutPort::availableDevices() const
{
    std::lock_guard lock(m_devicesMutex);

    std::vector<MidiDevice> ret;
#if JACK_AUDIO
    auto vj = m_midiOutPortJack->availableDevices();
    ret.insert(ret.end(), vj.begin(), vj.end());
#endif
    auto va = m_midiOutPortAlsa->availableDevices();
    ret.insert(ret.end(), va.begin(), va.end());

    // FIX: this should be done by gui/caller
    ret.push_back({ NONE_DEVICE_ID, trc("midi", "No device") });
    return ret;
}

mu::async::Notification LinuxMidiOutPort::availableDevicesChanged() const
{
    LOGI("---- linux availableDevicesChanged ----");
    return m_availableDevicesChanged;
}

mu::Ret LinuxMidiOutPort::connect(const MidiDeviceID& deviceID)
{
    LOGI("---- linux connect to %s ----", deviceID.c_str());
    if (!deviceExists(deviceID)) {
        return make_ret(Err::MidiFailedConnect, "not found device, id: " + deviceID);
    }
    
    DEFER {
        m_deviceChanged.notify();
    };

    if (!deviceID.empty() && deviceID != NONE_DEVICE_ID) {
        std::vector<int> deviceParams = splitDeviceId(deviceID);
        IF_ASSERT_FAILED(deviceParams.size() == 3) {
            return make_ret(Err::MidiInvalidDeviceID, "invalid device id: " + deviceID);
        }
        
        if (deviceParams.at(1) == 9999) { // This is an jack device
            LOGI("---- linux connect to jack ----");
            m_midiOutPortCurrent = /* JackMidiOutPort */ m_midiOutPortJack.get();
        } else {
            LOGI("---- linux connect to alsa ----");
            m_midiOutPortCurrent = /* AlsaMidiOutPort */ m_midiOutPortAlsa.get();
        }

        if (m_midiOutPortCurrent->isConnected()) {
            m_midiOutPortCurrent->disconnect();
        }
        LOGD() << "Connected to " << deviceID; // FIX: let caller log instead (has return state of connect)
        m_deviceID = deviceID;
        // continue the connect in the driver
        return m_midiOutPortCurrent ->connect(deviceID);
    }

    return Ret(true);
}

void LinuxMidiOutPort::disconnect()
{
    LOGI("---- linux disconnect ----");
    if (!isConnected()) {
        return;
    }

    LOGD() << "Disconnected from " << m_deviceID;

    //m_alsa->client = -1;
    //m_alsa->port = -1;
    //m_alsa->midiOut = nullptr;
    m_deviceID.clear();
}

bool LinuxMidiOutPort::isConnected() const
{
    LOGI("---- linux isConnect ----");
    return m_midiOutPortCurrent && !m_deviceID.empty();
}

MidiDeviceID LinuxMidiOutPort::deviceID() const
{
    LOGI("---- linux deviceID ----");
    return m_deviceID;
}

mu::async::Notification LinuxMidiOutPort::deviceChanged() const
{
    LOGI("---- linux deviceChanged ----");
    return m_deviceChanged;
}

bool LinuxMidiOutPort::supportsMIDI20Output() const
{
    LOGI("---- linux supportsMIDI20Output ----");
    return false;
}


mu::Ret LinuxMidiOutPort::sendEvent(const Event& e)
{
    // LOGI() << e.to_string();
    LOGI() << "---- linux sendEvent ----" << e.to_string();

    if (!isConnected()) {
        LOGI() << "---- linux sendEvent NOT CONNECTED";
        return make_ret(Err::MidiNotConnected);
    }

    if (e.isChannelVoice20()) {
        auto events = e.toMIDI10();
        for (auto& event : events) {
            mu::Ret ret = sendEvent(event);
            if (!ret) {
                return ret;
            }
        }
        return Ret(true);
    }

    LOGI() << "---- linux sendEvent to driver ----" << e.to_string();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    Event e2(e);
    return Ret(m_audioModule->getDriver()->pushMidiEvent(e2));
#else // alsa
    return m_midiOutPortCurrent->sendEvent(e);
#endif
}

bool LinuxMidiOutPort::deviceExists(const MidiDeviceID& deviceId) const
{
    LOGI("---- linux deviceExists ----");
    for (const MidiDevice& device : availableDevices()) {
        LOGI("---- linux deviceExists dev equal? %s <=> %s", deviceId.c_str(), device.id.c_str());
        if (device.id == deviceId) {
            return true;
        }
    }
    return false;
}

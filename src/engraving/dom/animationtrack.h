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

#ifndef __ANIMATIONTRACK_H__
#define __ANIMATIONTRACK_H__

#include "engravingitem.h"
#include "mscore.h"

namespace mu::engraving {
class Staff;
class AnimationKey;

class AnimationTrack
{
    std::vector<AnimationKey*> _keys;

public:
    AnimationTrack(EngravingItem* parent);

    virtual ~AnimationTrack();

    AnimationTrack& operator=(const BarLine&) = delete;

    const std::vector<AnimationKey*>& keys() const { return _keys; }
    std::vector<AnimationKey*>& keys() { return _keys; }
    // void addKey(AnimationKey* vertex);
    void addKey(Fraction tick, float value);
    void removeKey(Fraction tick);
};
}

#endif // __ANIMATIONTRACK_H__

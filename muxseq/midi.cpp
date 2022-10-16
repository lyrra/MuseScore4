#include "libmscore/types.h"
#include "libmscore/musescoreCore.h"
#include "libmscore/score.h"
#include "libmscore/staff.h"
#include "libmscore/chord.h"
#include "libmscore/part.h"
#include "libmscore/tie.h"

#include "muxseqclient.h"

namespace Ms {

//---------------------------------------------------------
//   processMidiInput
//---------------------------------------------------------

std::vector<struct MidinoteStart*> processMidiInput(Score* score)
      {
      QQueue<MidiInputEvent>* midiInputQueue = score->midiInputQueue();
      const InputState& _is = score->inputState();
      std::list<MidiInputEvent>* activeMidiPitches = score->activeMidiPitches();
      const Selection& selection = score->selection();
      int nstaves = score->nstaves();
      bool noteEntryMode = score->noteEntryMode();

      std::vector<struct MidinoteStart*> notes;
      if (midiInputQueue->empty()) {
            return notes;
            }
      NoteEntryMethod entryMethod = _is.noteEntryMethod();
      while (!midiInputQueue->empty()) {
            MidiInputEvent ev = midiInputQueue->dequeue();
            for (auto itr = activeMidiPitches->begin(); itr != activeMidiPitches->end();) {
                  if ((*itr).pitch == ev.pitch)
                        itr = activeMidiPitches->erase(itr);
                  else
                        ++itr;
                  }
            if (!noteEntryMode
                        || entryMethod == NoteEntryMethod::REALTIME_AUTO
                        || entryMethod == NoteEntryMethod::REALTIME_MANUAL) {
                  int staffIdx = selection.staffStart();
                  Part* p;
                  if (staffIdx < 0 || staffIdx >= nstaves)
                        p = score->staff(0)->part();
                  else
                        p = score->staff(staffIdx)->part();
                  if (p) {
                        if (!score->styleB(Sid::concertPitch)) {
                              ev.pitch += p->instrument(selection.tickStart())->transpose().chromatic;
                              }
                        MidinoteStart *note = (MidinoteStart*) malloc(sizeof(struct MidinoteStart));
                        note->channel = p->instrument(selection.tickStart())->channel(0)->channel();
                        note->pitch = ev.pitch;
                        note->velo = ev.velocity;
                        note->x = 0.0;
                        notes.push_back(note);
                        }
                  }
            if (noteEntryMode) {
                  if (ev.velocity == 0) {
                        // delete note in realtime mode
                        //Chord* chord = toChord(_is.cr());
                        //std::vector<Note*> notes = chord->notes();
                        if (entryMethod == NoteEntryMethod::REALTIME_AUTO || entryMethod == NoteEntryMethod::REALTIME_MANUAL) {
                              if (_is.cr()->isChord()) {
                                    Note* n = toChord(_is.cr())->findNote(ev.pitch);
                                    if (n) {
                                          score->maybeStartCmd();
                                          score->deleteItem(n->tieBack());
                                          score->deleteItem(n);
                                          }
                                    }
                              }
                        continue;
                        }
                  score->maybeStartCmd();
                  if (activeMidiPitches->empty())
                        ev.chord = false;
                  else
                        ev.chord = true;

                  // holding shift while inputting midi will add the new pitch to the prior existing chord
                  if (qApp->keyboardModifiers() & Qt::ShiftModifier) {
                        Element* cr = _is.lastSegment()->element(_is.track());
                        if (cr && cr->isChord())
                              ev.chord = true;
                        }

                  // TODO: add shadow note instead of real note in realtime modes
                  // (note becomes real when realtime-advance triggered).
                  score->addMidiPitch(ev.pitch, ev.chord);
                  activeMidiPitches->push_back(ev);
                  }
            }
      return notes;
      }

}


namespace Ms {

struct MidinoteStart {
    int channel;
    int pitch;
    int velo;
    float x;
};

std::vector<struct MidinoteStart*> processMidiInput(Score* score);

 /*
std::vector<struct MidinoteStart*>
processMidiInput(QQueue<MidiInputEvent>* midiInputQueue,
                 const InputState& _is,
                 std::list<MidiInputEvent>* activeMidiPitches,
                 const Selection& selection,
                 Score* score,
                 int nstaves,
                 bool noteEntryMode,
                 bool *retSomeBool // out-parameter, ugly
                 );
                 */

}

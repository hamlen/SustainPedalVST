# SustainPedalVST

*SustainPedal* is a VST3 that delays note-off MIDI messages received while the sustain pedal (MIDI CC 64) is pressed until the sustain pedal is released. Recipients of *SustainPedal*'s MIDI stream will therefore only see a stream of MIDI note-on and note-off events without any explicit sustain pedal messages.

This is useful for communicating with instruments that misinterpret or mishandle sustain pedal events. For example, if you press and release the same note ten times while holding down the sustain pedal on your keyboard, many virtual synthesizers respond by playing ten independent samples of that note simultaneously, resulting in a sound that is ten times louder than desired (usually resulting in audio clipping or possibly even damaging your speakers). Other virtual instruments completely ignore sustain pedal events. Such problems can be circumvented by filtering the MIDI stream through *SustainPedal* (i.e., sending the incoming MIDI stream to *SustainPedal*, and sending *SustainPedal*'s output MIDI stream to the virtual instrument).

To control what happens when *SustainPedal* receives a MIDI note-on event for a note that is already sounding (e.g., because the note was previously pressed while the sustain pedal was down, and the sustain pedal has not yet been released), *SustainPedal* exports an automation parameter named **Retrigger**. When **Retrigger** is on (set to 1.0), *SustainPedal* releases and then retriggers the note. When **Retrigger** is off (set to 0.0), *SustainPedal* just ignores the duplicate note, letting the existing note sustain until the pedal is released.

*SustainPedal* listens to all 16 MIDI channels independently, interpreting their sustain pedal events and delaying their note-off events accordingly. The condition of all 16 sustain pedals is reported as automation parameters named **Pedal1** through **Pedal16**.

Since *SustainPedal* is a VST3, it ignores (and does not resend) incoming MIDI CC messages other than note events (where note events include note-on, note-off, note pressure, and note expression events). To ensure your instrument gets any non-note events you may want it to receive, you should therefore create a separate MIDI route for those events in your DAW (and remember to filter out note events and sustain pedal events from it, since those are handled by the route coming from *SustainPedal*).

# SustainPedalVST

*SustainPedal* is a VST3 that suppresses and saves note-off MIDI messages received while the damper pedal (MIDI CC 64) is pressed, or that are being selectively sustained by the sostenuto pedal (MIDI CC 66), eventually transmitting them once the pedal is released. Recipients of *SustainPedal*'s MIDI stream therefore only see a stream of MIDI note-on and note-off events without any explicit pedal messages.

This is useful for communicating with instruments that misinterpret or mishandle pedal events. For example, sending a sustain pedal-down message to a virtual instrument followed by ten presses and releases of the same note can cause some instruments to play ten independent samples of that note simultaneously, resulting in a sound that is ten times louder than desired (usually resulting in audio clipping or possibly even damaging your speakers). Other virtual instruments completely ignore pedal events. Such problems can be circumvented by filtering the MIDI stream through *SustainPedal* (i.e., sending the incoming MIDI stream to *SustainPedal*, and sending *SustainPedal*'s output MIDI stream to the virtual instrument).

To control what happens when *SustainPedal* receives a MIDI note-on event for a pitch that is already sounding (e.g., because a previous note of that pitch is still being sustained by a pedal), *SustainPedal* exports an automation parameter named **Retrigger**. When **Retrigger** is on (set to 1.0), *SustainPedal* releases and then retriggers the note. When **Retrigger** is off (set to 0.0), *SustainPedal* just ignores the duplicate note, letting the existing note sustain until the pedal is released.

*SustainPedal* listens to all 16 MIDI channels independently, interpreting their pedal events and delaying their note-off events accordingly. The condition of all 16 sustain pedals and sostenuto pedals is reported as automation parameters named **Sustain1** ... **Sustain16** and **Sostenuto1** ... **Sostenuto16**, respectively.

Since *SustainPedal* is a VST3, it ignores (and does not resend) incoming MIDI CC messages other than pedal and note events (including note-on, note-off, note pressure, and note expression events). To ensure your instrument gets any non-note events you may want it to receive, you should therefore create a separate MIDI route for those events in your DAW (and remember to filter out note events and pedal events from it, since those are handled by the route coming from *SustainPedal*).

### Changes

* v1.0: initial release
* v1.1: sostenuto pedal added
* v1.2: improved sample-accuracy and stuck note avoidance
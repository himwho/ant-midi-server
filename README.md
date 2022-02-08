# ant-midi-server
Dockerized server for listening to LAN MIDI and broadcasting it to WAN

## Components
 - C++ OpenFrameworks controller app for handling and broadcasting multiple ant colonies
 - Custom formicarium embedded with various sensors (first generations focused on LDRs): [CAD Design Directory](custom-formicarium/cad/antformicarium-assembled)
 - Circuit for handling sensors, circuit allows analog -5v->+5v CV output as well as shield mounting for Arduino Uno form factor boards: [Circuit Design Directory](custom-formicarium/circuit-design/)
 - C Controller code for digital ADV handling for the sensors: [Arduino Controller Code](arduino-controller/ant-arduino-6i-controller.ino)
 - (Temporary) Max patch for receiving all arduino controllers and pushing them forward as OSC for web component: [Max Patch Controller](max-controller/AntPlayer.maxpat)
 - NodeJS handler receiving OSC on LAN and pushing socket.io as an OSC stream
 - Client side static website receving stream and outputting MIDI for any local device

## Usage
Server listens for messages via OSC and sends them to any connecting client via HTTP

## TODO List
- parse seperate input devices for multichannel MIDI
- stylize the web component
- cleanup the max patch
- design arduino patch to output OSC directly

## Notes
Moving OSC handling to arduino controller side may be an issue due to lack of multithreading already for parsing sensors

usbmodem14601 = hab001 = Brown Camponotus P. Hab
usbmodem1434301 = hab002 = Clear Camponotus P. Hab
usbmodem = hab003 = Tan Camponotus P. Hab

## Ant Journal
I originally chose ants for this thinking they would be easy to handle and grow into large numbers making them ideal for entropy and synthesizing perceived randomness. It turns out its very challenging to get a queen ant to properly pass the foundling stage and grow into a sustaining colony. Below will be some notes and issues I did not expect more specific to this project.

#### Light Sesnsitivity
Using LDR (light) based sensors is not ideal, queen ants tend to get stressed out from light, part of the project included training the queen ant(s) to get used to light to keep this project running longer term. Queen ants that did not have a light cover showed at least a 50% decrease in colony size over time. I only started light training for a colony when they got to a certain size (generally over 15 workers).

#### Colony Growth
I picked Camponotus Pennsylvanicus because they were common and enormous. However I did not realize this species of ant is slower to grow in colony size, I originally expected an ant colony to take weeks at most, but this extended the project to a 2-3 year timeline.

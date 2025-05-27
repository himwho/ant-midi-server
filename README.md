# Ant MIDI Server

A Node.js server that receives OSC messages from ant colony sensors and converts them to MIDI output, with real-time web interface and encryption capabilities.

## Features

- OSC to MIDI conversion
- Real-time web interface with Vue.js
- MIDI device selection and output
- Web Audio API synthesizer
- Security dashboard with hash generation
- File encryption using ant-generated entropy
- Socket.io real-time communication

## Installation

```bash
npm install
npm start
```

The server will start on port 3000 and listen for OSC messages on port 9998.

## Troubleshooting

### Common Issues

1. **"AntCrypt is not defined" error**
   - Make sure `public/js/encryption.js` is loaded before the main Vue app
   - Check browser console for script loading errors

2. **MIDI devices not showing**
   - Ensure browser supports Web MIDI API (Chrome/Edge recommended)
   - Check browser permissions for MIDI access
   - Look for MIDI access logs in browser console

3. **Socket.io connection errors (502)**
   - Check if server is running on correct port
   - Verify proxy/hosting configuration
   - Updated to socket.io v4.7.5 for better compatibility

4. **Vue template variables showing as `{{ }}`**
   - Indicates Vue.js failed to initialize
   - Check for JavaScript errors in console
   - Ensure all required scripts are loaded

5. **AudioContext warnings**
   - Modern browsers require user interaction before audio
   - Click on the page before using audio features
   - Consider using AudioWorkletNode instead of ScriptProcessorNode

### Development

- Main server: `main.js`
- Frontend: `index.html`
- Security dashboard: `security.html`
- Client scripts: `public/js/`

### File Structure

```
├── main.js              # Express server with Socket.io and OSC
├── index.html           # Main web interface
├── security.html        # Security dashboard
├── package.json         # Dependencies
└── public/
    ├── css/
    └── js/
        ├── encryption.js      # AntCrypt class for file encryption
        ├── midi.js           # MIDI handling
        ├── security-dashboard.js # Security hash generation
        ├── synth.js          # Web Audio synthesizer
        └── ui.js             # UI utilities
```

## API Endpoints

- `GET /` - Main dashboard
- `GET /security` - Security dashboard
- `GET /public/*` - Static assets

## Socket.io Events

- `osc` - OSC message data (x, y, z values)
- `internal` - Internal system data
- `hash` - Security hash updates

## OSC Integration

Send OSC messages to port 9998 with 3 float arguments:
- `/default` - General sensor data
- `/hash` - Security hash updates

## Components
 - C++ OpenFrameworks controller app for handling and broadcasting multiple ant colonie: [Controller Directory](desktop-controller)
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
- workout a feedback design for audio to vibrate safely the colony enclosures

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

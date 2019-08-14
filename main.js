
const express = require('express')
const app = express();

var jazz = require('jazz-midi');
var midi = new jazz.MIDI();

var out_name;
var in_name;
var current;
var delta;

app.get('/', (req, res) => {
	res.send('Listening for midi...')
	 
	start_recording();
});

app.listen(8000, () => {
  console.log('Listening on port 8000!')
});

function start_recording(){
  out_name = midi.MidiOutOpen(0);
  if(out_name==''){ console.log('No default MIDI-Out port!'); return;}
  in_name = midi.MidiInOpen(0);
  if(in_name==''){ console.log('No default MIDI-In port!'); return;}
  console.log('Recording from', in_name, '...');
  setTimeout(start_playing, 5000);
}

function start_playing(){
  midi.MidiInClose();
  console.log('Playing to', out_name, '...');
  current = midi.QueryMidiIn();
  if(!current){
    cleanup();
    return;
  }
  delta = midi.Time() - current[0];
  next_msg();
}

function next_msg(){
  if(!current){
    setTimeout(cleanup, 1000);
    return;
  }
  var wait = current[0] + delta - midi.Time();
  if(wait<=0){
    midi.MidiOutLong(current.slice(1, current.length));
    current = midi.QueryMidiIn();
    next_msg();
  }
  else setTimeout(next_msg, wait);
}

function cleanup(){
  midi.MidiOutClose();
  console.log('Hope you have enjoyed!');
}
